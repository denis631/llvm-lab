#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"

#include "global.h"
#include "callstring.h"
#include "value_set.h"

namespace pcpo {

// Run the simple fixpoint algorithm with callstrings. AbstractState should implement the interface
// documented in AbstractStateDummy (no need to subclass or any of that, just implement the methods
// with the right signatures and take care to fulfil the contracts outlines above).
// Note that a lot of this code is duplicated in executeFixpointAlgorithmWidening in
// fixpoint_widening.cpp, so if you fix any bugs in here, they probably should be fixed there as
// well.
//  Tip: Look at a diff of fixpoint.cpp and fixpoint_widening.cpp with a visual diff tool (I
// recommend Meld.)
template <typename AbstractState>
void executeFixpointAlgorithmTwoVarEq(llvm::Module const& M) {
    constexpr int iterations_max = 1000;

    // A node in the control flow graph, i.e. a basic block. Here, we need a bit of additional data
    // per node to execute the fixpoint algorithm.
    struct Node {
        llvm::BasicBlock const* bb;
        llvm::BasicBlock const* callstring;
        AbstractState state;
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for initial basic
        // blocks.
        llvm::Function const* func_entry = nullptr;
    };

    std::unordered_map<bb_key, Node> nodes;
    std::vector<bb_key> worklist; // Contains the tuples of basic block and callstring, which is the key of a node, that need to be processed

    // We only consider the main function in the beginning. If no main exists, nothing is evaluated!
    llvm::Function const* main_func = M.getFunction("main");

    //creating dummy block for callstrings of the main block, since the main function is not called from another function
    llvm::BasicBlock const* dummy_block = llvm::BasicBlock::Create(M.getContext(), "dummy");

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    // Register basic blocks of the main function
    for (llvm::BasicBlock const& bb: *main_func) {
        dbgs(1) << "  Found basic block main." << bb.getName() << '\n';

        Node node;
        node.bb = &bb;
        node.callstring = dummy_block;
        // node.state is default initialised (to bottom)

        // nodes of main block have the callstring of a dummy block
        nodes[std::make_tuple(&bb, dummy_block)] = node;
    }

    // Push the initial block into the worklist
    auto init_element = std::make_tuple(&main_func->getEntryBlock(), dummy_block);
    worklist.push_back(init_element);
    nodes[init_element].update_scheduled = true;
    nodes[init_element].state = AbstractState {*main_func};
    nodes[init_element].func_entry = main_func;

    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n";

    for (int iter = 0; !worklist.empty() and iter < iterations_max; ++iter) {
        Node& node = nodes[worklist.back()];
        worklist.pop_back();
        node.update_scheduled = false;

        dbgs(1) << "\nIteration " << iter << ", considering basic block "
                << _bb_to_str(node.bb) << " with callstring "
                << _bb_to_str(node.callstring) << '\n';

        AbstractState state_new; // Set to bottom

        if (node.func_entry) {
            dbgs(1) << "  Merging function parameters, is entry block\n";

            // if it is the entry node, then its state should be top
            state_new.isBottom = false;
            state_new.merge(Merge_op::UPPER_BOUND, node.state);
        }

        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        std::vector<AbstractState> predecessors;
        for (llvm::BasicBlock const* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << _bb_to_str(bb) << '\n';

            AbstractState state_branched {nodes[std::make_tuple(bb, node.callstring)].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(Merge_op::UPPER_BOUND, state_branched);
            predecessors.push_back(state_branched);
        }

        dbgs(2) << "  Relevant incoming state is:\n"; state_new.printIncoming(*node.bb, dbgs(2), 4);

        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";

        if (state_new.isBottom) {
            dbgs(3) << "    Basic block is unreachable, everything is bottom\n";
        } else {
            // Applies all instrucions of a basic block
            for (llvm::Instruction const& inst: *node.bb) {

                // Handles return instructions
                if (llvm::dyn_cast<llvm::ReturnInst>(&inst)) {
                    state_new.applyReturnInst(inst);
                }

                // If the result of the instruction is not used, there is no reason to compute
                // it. (There are no side-effects in LLVM IR. (I hope.))
                if (inst.use_empty()) {
                    // Except for call instructions, we still want to get that information
                    if (not llvm::dyn_cast<llvm::CallInst>(&inst)) {
                        dbgs(3) << "    Empty use of instruction, skipping...\n";
                        continue;
                    }
                }

                // Handles merging points
                if (llvm::dyn_cast<llvm::PHINode>(&inst)) {

                    state_new.applyPHINode(*node.bb, predecessors, inst);

                // Handles function calls
                } else if (llvm::CallInst const* call = llvm::dyn_cast<llvm::CallInst>(&inst)) {

                    // Checks if an input parameter for the callee is bottom. If so,
                    // then skip the calculation of the call instruction for now
                    if (state_new.checkOperandsForBottom(inst)) continue;

                    llvm::Function const* callee_func = call->getCalledFunction();

                    // Checks for functions, such as printf and skips them
                    if (callee_func->empty()) {
                        dbgs(3) << "    Function " << callee_func->getName() << " is external, skipping...\n";
                        continue;
                    }

                    auto callee_element = std::make_tuple(&callee_func->getEntryBlock(), node.bb);
                    bool changed;

                    // Checks whether a node with key [%callee entry block, %caller basic block],
                    // i.e. an entry block with callstring of caller basic block, exists.
                    // If not, all nodes with their corrosponding keys are initilized for the callee function.
                    if (nodes.find(callee_element) == nodes.end()) {
                        // Check if abstract_state of call.bb is bottom or not
                        dbgs(3) << "    No information regarding function call %" << call->getCalledFunction()->getName() << "\n";

                        // Register basic blocks
                        for (llvm::BasicBlock const& bb : *callee_func) {
                            dbgs(4) << "      Found basic block " << _bb_to_str(&bb) << '\n';

                            Node callee_node;
                            callee_node.bb = &bb;
                            callee_node.callstring = node.bb;
                            // node.state is default initialised (to bottom)

                            nodes[std::make_tuple(&bb, node.bb)] = callee_node;
                        }

                        nodes[callee_element].state = AbstractState{ callee_func, state_new, call };
                        nodes[callee_element].func_entry = callee_func;
                        changed = true;
                    } else {
                        AbstractState state_update{ callee_func, state_new, call };
                        changed = nodes[callee_element].state.merge(Merge_op::UPPER_BOUND, state_update);
                    }

                    //Getting the last block
                    llvm::BasicBlock const* end_block = &*std::prev(callee_func->end());
                    auto end_element = std::make_tuple(end_block, node.bb);

                    state_new.applyCallInst(inst, end_block, nodes[end_element].state);

                    // If input parameters have changed, we want to interpret the function once again
                    // and reevaluate the nodes of possible callers.
                    if (changed) {
                        for (std::pair<const bb_key, Node>& i : nodes) {
                            if (std::get<0>(i.first) == node.bb and not i.second.update_scheduled) {
                                dbgs(3) << "      Adding possible caller " << _bb_key_to_str(i.first) << " to worklist\n";
                                worklist.push_back(i.first);
                                i.second.update_scheduled = true;
                            }
                        }
                        
                        // Checks if the key of the callee functions entry node is already on the worklist,
                        // this is necessary for recursions.
                        if (not nodes[callee_element].update_scheduled) {
                            worklist.push_back(callee_element);
                            nodes[callee_element].update_scheduled = true;
                            
                            dbgs(3) << "      Adding callee " << _bb_key_to_str(callee_element) << " to worklist\n";
                        } else {
                            dbgs(3) << "      Callee already on worklist, nothing to add...\n";
                        }
                    }
                } else {
                    if (state_new.checkOperandsForBottom(inst)) continue;
                    state_new.applyDefault(inst);
                }
            }
        }

        // Merge the state back into the node
        dbgs(3) << "  Merging with stored state\n";
        bool changed = node.state.merge(Merge_op::UPPER_BOUND, state_new);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.bb, dbgs(2), 4);

        // No changes, so no need to do anything else
        if (not changed) continue;

        dbgs(2) << "  State changed, notifying " << llvm::succ_size(node.bb)
                << (llvm::succ_size(node.bb) != 1 ? " successors\n" : " successor\n");

        // Something changed and we will need to update the successors
        for (llvm::BasicBlock const* succ_bb: llvm::successors(node.bb)) {
            auto succ_key = std::make_tuple(succ_bb, node.callstring);
            Node& succ = nodes[succ_key];
            if (not succ.update_scheduled) {
                worklist.push_back(succ_key);
                succ.update_scheduled = true;

                dbgs(3) << "    Adding " << _bb_key_to_str(succ_key) << " to worklist\n";
            }
        }
    }

    if (!worklist.empty()) {
        dbgs(0) << "Iteration terminated due to exceeding loop count.\n";
    }
     
    // Output the final result
    dbgs(0) << "\nFinal result:\n";
    for (std::pair<bb_key, Node> i: nodes) {
        dbgs(0) << _bb_key_to_str(i.first) << ":\n";
        i.second.state.printOutgoing(*i.second.bb, dbgs(0), 2);
    }

}

} /* end of namespace pcpo */

