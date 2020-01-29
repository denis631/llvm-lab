#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"

#include "general.h"
#include "global.h"
#include "callstring.h"

#include "value_set.h"
#include "simple_interval.h"
#include "normalized_conjunction.h"

#include "fixpoint_widening.cpp"
#include "fixpoint_two_var_eq.cpp"


namespace std {

template <typename ... T>
struct hash<tuple<T...>> {
    size_t operator()(tuple<T...> const& t) const {
        size_t seed = 0;
        apply([&seed](const auto&... item) {(( seed = llvm::hash_combine(seed, item) ), ...);}, t);
        return seed;
    }
};

template <typename T, typename U>
struct hash<pair<T, U>> {
    size_t operator()(pair<T, U> const& in) const {
        return llvm::hash_value(in);
    }
};


template <typename T>
struct hash<vector<T>> {
    size_t operator()(vector<T> const& in) const {
        return llvm::hash_combine_range(in.begin(), in.end());
    }
};

}

namespace llvm {

template <typename T>
static hash_code hash_value(std::vector<T> const& in) {
    return hash_combine_range(in.begin(), in.end());
}

}


namespace pcpo {

using namespace llvm;
using namespace std;

static llvm::RegisterPass<AbstractInterpretationPass> Y("painpass", "AbstractInterpretation Pass");

char AbstractInterpretationPass::ID;

int debug_level = DEBUG_LEVEL; // from global.hpp

class AbstractStateDummy {
public:
    // This has to initialise the state to bottom.
    AbstractStateDummy() = default;

    // Creates a copy of the state. Using the default copy-constructor should be fine here, but if
    // some members do something weird you maybe want to implement this as
    //     AbstractState().merge(state)
    AbstractStateDummy(AbstractStateDummy const& state) = default;

    // Initialise the state to the incoming state of the function. This should do something like
    // assuming the parameters can be anything.
    explicit AbstractStateDummy(llvm::Function const& f) {}

    // Initialise the state of a function call with parameters of the caller.
    // This is the "enter" function as described in "Compiler Design: Analysis and Transformation" 
    explicit AbstractStateDummy(llvm::Function const* callee_func, AbstractStateDummy const& state,
                                llvm::CallInst const* call) {}

    // Apply functions apply the changes needed to reflect executing the instructions in the basic block. Before
    // this operation is called, the state is the one upon entering bb, afterwards it should be (an
    // upper bound of) the state leaving the basic block.
    //  predecessors contains the outgoing state for all the predecessors, in the same order as they
    // are listed in llvm::predecessors(bb).

    // Applies instructions within the PHI node, needed for merging
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<AbstractStateDummy> const& pred_values,
                      llvm::Instruction const& inst) {};

    // This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block,
                       AbstractStateDummy const& callee_state) {};

    // Evaluates return instructions, needed for the main function and the debug output
    void applyReturnInst(llvm::Instruction const& inst) {};

    // Handles all cases different from the three above
    void applyDefault(llvm::Instruction const& inst) {};

    // This 'merges' two states, which is the operation we do fixpoint iteration over. Currently,
    // there are three possibilities for op:
    //   1. UPPER_BOUND: This has to return some upper bound of itself and other, with more precise
    //      bounds being preferred.
    //   2. WIDEN: Same as UPPER_BOUND, but this operation should sacrifice precision to converge
    //      quickly. Returning T would be fine, though maybe not optimal. For example for intervals,
    //      an implementation could ensure to double the size of the interval.
    //   3. NARROW: Return a value between the intersection of the state and other, and the
    //      state. In pseudocode:
    //          intersect(state, other) <= narrow(state, other) <= state
    // For all of the above, this operation returns whether the state changed as a result.
    // IMPORTANT: The simple fixpoint algorithm only performs UPPER_BOUND, so you do not need to
    // implement the others if you just use that one. (The more advanced algorithm in
    // fixpoint_widening.cpp uses all three operations.)
    bool merge(Merge_op::Type op, AbstractStateDummy const& other) { return false; };

    // Restrict the set of values to the one that allows 'from' to branch towards
    // 'towards'. Starting with the state when exiting from, this should compute (an upper bound of)
    // the possible values that would reach the block towards. Doing nothing thus is a valid
    // implementation.
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) {};

    // Functions to generate the debug output. printIncoming should output the state as of entering
    // the basic block, printOutcoming the state when leaving it.
    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
};

using Callstring = vector<BasicBlock const *>;
using NodeKey = pair<Callstring, BasicBlock const *>;

template <typename AbstractState>
struct Node {
    BasicBlock const* basic_block;
    Callstring callstring;
    AbstractState state = {};
    bool update_scheduled = false; // Whether the node is already in the worklist

    // If this is set, the algorithm will add the initial values from the parameters of the
    // function to the incoming values, which is the correct thing to do for initial basic
    // blocks.
    Function const* func_entry = nullptr;
};


// Run the simple fixpoint algorithm with callstrings. AbstractState should implement the interface 
// documented in AbstractStateDummy (no need to subclass or any of that, just implement the methods
// with the right signatures and take care to fulfil the contracts outlines above).
// Note that a lot of this code is duplicated in executeFixpointAlgorithmWidening in
// fixpoint_widening.cpp, so if you fix any bugs in here, they probably should be fixed there as
// well.
//  Tip: Look at a diff of fixpoint.cpp and fixpoint_widening.cpp with a visual diff tool (I
// recommend Meld.)
template<typename AbstractState,
    int iterations_max = 1000,
    int callstack_depth = 1,
    Merge_op::Type merge_op = Merge_op::UPPER_BOUND>
void executeFixpointAlgorithm(Module const& M) {
    using Node = Node<AbstractState>;

    // A node in the control flow graph, i.e. a basic block. Here, we need a bit of additional data
    // per node to execute the fixpoint algorithm.

    unordered_map<NodeKey, Node> nodes;
    vector<Node *> worklist;

    // We only consider the main function in the beginning. If no main exists, nothing is evaluated!
    Function const* main_func = M.getFunction("main");

    //creating dummy block for callstrings of the main block, since the main function is not called from another function
    BasicBlock const* dummy_block = BasicBlock::Create(M.getContext(), "dummy");

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    // Register basic blocks of the main function
    for (BasicBlock const& bb: *main_func) {
        dbgs(1) << "  Found basic block main." << bb.getName() << '\n';

        Node node;
        node.basic_block = &bb;
        node.callstring = {dummy_block};
        // node.state is default initialised (to bottom)

        // nodes of main block have the callstring of a dummy block
        nodes[{{dummy_block}, &bb}] = node;
    }

    // Push the initial block into the worklist
    NodeKey init_element = {{dummy_block}, &main_func->getEntryBlock()};
    worklist.push_back(&nodes[init_element]);
    nodes[init_element].update_scheduled = true;
    nodes[init_element].state = AbstractState {*main_func};
    nodes[init_element].func_entry = main_func;

    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n";

    for (int iter = 0; !worklist.empty() and iter < iterations_max; ++iter) {
        Node& node = *worklist.back();
        worklist.pop_back();
        node.update_scheduled = false;

        dbgs(1) << "\nIteration " << iter << ", considering basic block "
                << _bb_to_str(node.basic_block) << " with callstring "
                << "_bb_to_str(node.callstring)" << '\n';

        AbstractState state_new; // Set to bottom

        if (node.func_entry) {
            dbgs(1) << "  Merging function parameters, is entry block\n";

            // if it is the entry node, then its state should be top
            state_new.isBottom = false;
            state_new.merge(merge_op, node.state);
        }

        dbgs(1) << "  Merge of " << pred_size(node.basic_block)
                << (pred_size(node.basic_block) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        vector<AbstractState> predecessors;
        for (BasicBlock const* bb: llvm::predecessors(node.basic_block)) {
            dbgs(3) << "    Merging basic block " << _bb_to_str(bb) << '\n';

            AbstractState state_branched {nodes[{{node.callstring}, bb}].state};
            state_branched.branch(*bb, *node.basic_block);
            state_new.merge(merge_op, state_branched);
            predecessors.push_back(state_branched);
        }

        dbgs(2) << "  Relevant incoming state is:\n"; state_new.printIncoming(*node.basic_block, dbgs(2), 4);

        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";

        if (state_new.isBottom) {
            dbgs(3) << "    Basic block is unreachable, everything is bottom\n";
        } else {
            // Applies all instrucions of a basic block
            for (Instruction const& inst: *node.basic_block) {

                // Handles return instructions
                if (isa<ReturnInst>(&inst)) {
                    state_new.applyReturnInst(inst);
                }

                // If the result of the instruction is not used, there is no reason to compute
                // it. (There are no side-effects in LLVM IR. (I hope.))
                if (inst.use_empty()) {
                    // Except for call instructions, we still want to get that information
                    if (not isa<CallInst>(&inst)) {
                        dbgs(3) << "    Empty use of instruction, skipping...\n";
                        continue;
                    }
                }

                // Handles merging points
                if (isa<PHINode>(&inst)) {

                    state_new.applyPHINode(*node.basic_block, predecessors, inst);

                // Handles function calls 
                } else if (CallInst const* call = dyn_cast<CallInst>(&inst)) {

                    // Checks if an input parameter for the callee is bottom. If so,
                    // then skip the calculation of the call instruction for now
                    if (state_new.checkOperandsForBottom(inst)) continue;

                    Function const* callee_func = call->getCalledFunction();

                    // Checks for functions, such as printf and skips them
                    if (callee_func->empty()) {
                        dbgs(3) << "    Function " << callee_func->getName() << " is external, skipping...\n";
                        continue;
                    }

                    NodeKey callee_element = {{node.basic_block}, &callee_func->getEntryBlock()};
                    bool changed;

                    // Checks whether a node with key [%callee entry block, %caller basic block],
                    // i.e. an entry block with callstring of caller basic block, exists.
                    // If not, all nodes with their corrosponding keys are initilized for the callee function.
                    if (nodes.find(callee_element) == nodes.end()) { 
                        // Check if abstract_state of call.bb is bottom or not
                        dbgs(3) << "    No information regarding function call %" << call->getCalledFunction()->getName() << "\n";

                        // Register basic blocks
                        for (BasicBlock const& bb : *callee_func) {
                            dbgs(4) << "      Found basic block " << _bb_to_str(&bb) << '\n';

                            Node callee_node;
                            callee_node.basic_block = &bb;
                            callee_node.callstring = {node.basic_block};
                            // node.state is default initialised (to bottom)

                            nodes[{{node.basic_block}, &bb}] = callee_node;
                        }

                        nodes[callee_element].state = AbstractState{ callee_func, state_new, call };
                        nodes[callee_element].func_entry = callee_func;
                        changed = true;
                    } else {
                        AbstractState state_update{ callee_func, state_new, call };
                        changed = nodes[callee_element].state.merge(merge_op, state_update);
                    }

                    //Getting the last block
                    BasicBlock const* end_block = &*prev(callee_func->end());
                    NodeKey end_element = {{node.basic_block}, end_block};
                    state_new.applyCallInst(inst, end_block, nodes[end_element].state);

                    // If input parameters have changed, we want to interpret the function once again
                    // and reevaluate the nodes of possible callers.
                    if (changed) {
                        for (auto& [key, value]: nodes) {
                            if (key.first[0] == node.basic_block and not value.update_scheduled) {
                                dbgs(3) << "      Adding possible caller " << "_bb_key_to_str(i.first)" << " to worklist\n";
                                worklist.push_back(&value);
                                value.update_scheduled = true;
                            }
                        }
                        
                        // Checks if the key of the callee functions entry node is already on the worklist,
                        // this is necessary for recursions.
                        if (not nodes[callee_element].update_scheduled) {
                            auto& elem = nodes[callee_element];
                            worklist.push_back(&elem);
                            elem.update_scheduled = true;
                            
                            dbgs(3) << "      Adding callee " << "_bb_key_to_str(callee_element)" << " to worklist\n";
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
        bool changed = node.state.merge(merge_op, state_new);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.basic_block, dbgs(2), 4);

        // No changes, so no need to do anything else
        if (not changed) continue;

        dbgs(2) << "  State changed, notifying " << succ_size(node.basic_block)
                << (succ_size(node.basic_block) != 1 ? " successors\n" : " successor\n");

        // Something changed and we will need to update the successors
        for (BasicBlock const* succ_bb: successors(node.basic_block)) {
            NodeKey succ_key = {{node.callstring}, succ_bb};
            Node& succ = nodes[succ_key];
            if (not succ.update_scheduled) {
                worklist.push_back(&succ);
                succ.update_scheduled = true;

                dbgs(3) << "    Adding " << "_bb_key_to_str(succ_key)" << " to worklist\n";
            }
        }
    }

    if (!worklist.empty()) {
        dbgs(0) << "Iteration terminated due to exceeding loop count.\n";
    }
     
    // Output the final result
    dbgs(0) << "\nFinal result:\n";
    for (auto const& [key, node]: nodes) {
        dbgs(0) << "_bb_key_to_str(key)" << ":\n";
        node.state.printOutgoing(*node.basic_block, dbgs(0), 2);
    }

}

bool AbstractInterpretationPass::runOnModule(llvm::Module& M) {
    using AbstractState = AbstractStateValueSet<SimpleInterval>;

    // Use either the standard fixpoint algorithm or the version with widening
     executeFixpointAlgorithm<AbstractState>(M);
//    executeFixpointAlgorithmWidening<AbstractState>(M);

//    executeFixpointAlgorithmTwoVarEq<NormalizedConjunction>(M);

    // We never change anything
    return false;
}

void AbstractInterpretationPass::getAnalysisUsage(llvm::AnalysisUsage& info) const {
    info.setPreservesAll();
}


} /* end of namespace pcpo */

