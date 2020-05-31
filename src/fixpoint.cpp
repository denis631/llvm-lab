#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"

#include "global.h"

#include "constant_folding.h"
#include "linear_subspace.h"
#include "normalized_conjunction.h"
#include "simple_interval.h"
#include "value_set.h"

#include "fixpoint_widening.cpp"
#include "hash_utils.h"

#include "llvm/ADT/PostOrderIterator.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_os_ostream.h"
#include <fstream>
static llvm::cl::opt<std::string> OutputFilename(
    "vo", llvm::cl::desc("Specify the filename for the vizualization output"),
    llvm::cl::value_desc("filename"));

namespace pcpo {

using namespace llvm;
using std::ostream;
using std::pair;
using std::unordered_map;
using std::vector;

static llvm::RegisterPass<AbstractInterpretationPass>
    Y("painpass", "AbstractInterpretation Pass");

char AbstractInterpretationPass::ID;

int debug_level = DEBUG_LEVEL; // from global.hpp

using Callstring = vector<Function const *>;
using NodeKey = pair<Callstring, BasicBlock const *>;

// MARK: - To String

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              BasicBlock const &basic_block) {
  os << "%";
  if (llvm::Function const *f = basic_block.getParent()) {
    os << f->getName() << ".";
  }
  return os << basic_block.getName();
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              Callstring const &callstring) {
  for (auto call : callstring) {
    os << call->getName();
    if (call != callstring.back()) {
      os << " -> ";
    }
  }
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, NodeKey const &key) {
  return os << "[" << *key.second << "," << key.first << "]";
}

template <typename AbstractState> struct Node {
  BasicBlock const *basic_block;
  /// Function calls the lead to this basic block. The last element is always
  /// the current function.
  Callstring callstring;
  AbstractState state = {};
  bool update_scheduled = false; // Whether the node is already in the worklist

  /// Check wether this basic block is the entry block of its function.
  bool isEntry() const { return basic_block == &function()->getEntryBlock(); }

  /// Function in which this basic block is located.
  Function const *function() const { return callstring.back(); }
};

Callstring callstring_for(Function const *function,
                          Callstring const &callstring, int max_length) {
  Callstring new_callstring;
  for (auto call : callstring) {
    if (max_length-- > 0) {
      new_callstring.push_back(call);
    } else {
      return new_callstring;
    }
  }
  new_callstring.push_back(function);
  return new_callstring;
}

template <typename AbstractState>
vector<Node<AbstractState> *>
register_function(llvm::Function const *function, Callstring const &callstring,
                  int callstack_depth,
                  unordered_map<pcpo::NodeKey, Node<AbstractState>> &nodes) {
  Callstring new_callstring =
      callstring_for(function, callstring, callstack_depth);
  vector<Node<AbstractState> *> inserted_nodes;

  for (po_iterator<BasicBlock const *> I = po_begin(&function->getEntryBlock()),
                                       IE = po_end(&function->getEntryBlock());
       I != IE; ++I) {

    BasicBlock const *basic_block = *I;

    dbgs(1) << "  Found basic block: " << basic_block->getName() << '\n';
    NodeKey key = {new_callstring, basic_block};
    Node<AbstractState> node = {basic_block, new_callstring};
    if (node.isEntry()) {
      node.state = AbstractState{*node.function()};
    }
    inserted_nodes.push_back(&nodes[key]);
    nodes[key] = node;
  }
  return inserted_nodes;
}

template <typename AbstractState>
void add_to_worklist(vector<Node<AbstractState> *> &nodes,
                     vector<Node<AbstractState> *> &worklist) {
  for (Node<AbstractState> *node : nodes) {
    node->update_scheduled = true;
    worklist.push_back(node);
  }
}

// Run the simple fixpoint algorithm with callstrings. AbstractState should
// implement the interface documented in AbstractStateDummy (no need to subclass
// or any of that, just implement the methods with the right signatures and take
// care to fulfil the contracts outlines above). Note that a lot of this code is
// duplicated in executeFixpointAlgorithmWidening in fixpoint_widening.cpp, so
// if you fix any bugs in here, they probably should be fixed there as well.
//  Tip: Look at a diff of fixpoint.cpp and fixpoint_widening.cpp with a visual
//  diff tool (I
// recommend Meld.)
template <typename AbstractState, int iterations_max = 1000,
          int callstack_depth = 1,
          Merge_op::Type merge_op = Merge_op::UPPER_BOUND>
unordered_map<NodeKey, Node<AbstractState>>
executeFixpointAlgorithm(Module const &M) {
  using Node = Node<AbstractState>;

  // A node in the control flow graph, i.e. a basic block. Here, we need a bit
  // of additional data per node to execute the fixpoint algorithm.

  unordered_map<NodeKey, Node> nodes;
  vector<Node *> worklist;

  // We only consider the main function in the beginning. If no main exists,
  // nothing is evaluated!
  Function const *main_func = M.getFunction("main");

  // TODO: Check what this does for release clang, probably write out a warning
  dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

  // Register basic blocks of the main function
  auto main_basic_blocks =
      register_function(main_func, {}, callstack_depth, nodes);
  add_to_worklist(main_basic_blocks, worklist);

  dbgs(1) << "\nWorklist initialised with " << worklist.size()
          << (worklist.size() != 1 ? " entries" : " entry")
          << ". Starting fixpoint iteration...\n";

  for (int iter = 0; !worklist.empty() and iter < iterations_max; ++iter) {
    Node &node = *worklist.back();
    worklist.pop_back();
    node.update_scheduled = false;

    dbgs(1) << "\nIteration " << iter << ", considering basic block "
            << *node.basic_block << " with callstring " << node.callstring
            << '\n';

    AbstractState state_new; // Set to bottom

    if (node.isEntry()) {
      dbgs(1) << "  Merging function parameters, is entry block\n";

      // if it is the entry node, then its state should be top
      state_new.merge(merge_op, node.state);
      state_new.isBottom = false;
    }

    dbgs(1) << "  Merge of " << pred_size(node.basic_block)
            << (pred_size(node.basic_block) != 1 ? " predecessors.\n"
                                                 : " predecessor.\n");

    // Collect the predecessors
    vector<AbstractState> predecessors;
    for (BasicBlock const *basic_block : llvm::predecessors(node.basic_block)) {
      dbgs(3) << "    Merging basic block " << *basic_block << '\n';

      AbstractState state_branched{
          nodes[{{node.callstring}, basic_block}].state};
      state_branched.branch(*basic_block, *node.basic_block);
      state_new.merge(merge_op, state_branched);
      predecessors.push_back(state_branched);
    }

    dbgs(2) << "  Relevant incoming state is:\n";
    state_new.printIncoming(*node.basic_block, dbgs(2), 4);

    // Apply the basic block
    dbgs(3) << "  Applying basic block\n";

    if (state_new.isBottom) {
      dbgs(3) << "    Basic block is unreachable, everything is bottom\n";
    } else {
      // Applies all instrucions of a basic block
      for (Instruction const &inst : *node.basic_block) {

        // Handles return instructions
        if (isa<ReturnInst>(&inst)) {
          state_new.applyReturnInst(inst);
        }

        // If the result of the instruction is not used, there is no reason to
        // compute it. (There are no side-effects in LLVM IR. (I hope.))
        // if (inst.use_empty()) {
        //   // Except for call instructions, we still want to get that
        //   information if (not isa<CallInst>(&inst)) {
        //     dbgs(3) << "    Empty use of instruction, " <<
        //     inst.getOpcodeName()
        //             << " skipping...\n";
        //     continue;
        //   }
        // }

        // Handles merging points
        if (isa<PHINode>(&inst)) {
          state_new.applyPHINode(*node.basic_block, predecessors, inst);

          // Handles function calls
        } else if (CallInst const *call = dyn_cast<CallInst>(&inst)) {

          // Checks if an input parameter for the callee is bottom. If so,
          // then skip the calculation of the call instruction for now
          if (state_new.checkOperandsForBottom(inst))
            continue;

          Function const *callee_func = call->getCalledFunction();

          // Checks for functions, such as printf and skips them
          if (callee_func->empty()) {
            dbgs(3) << "    Function " << callee_func->getName()
                    << " is external, skipping...\n";
            continue;
          }

          Callstring new_callstring =
              callstring_for(callee_func, node.callstring, callstack_depth);

          NodeKey callee_element = {new_callstring,
                                    &callee_func->getEntryBlock()};
          vector<Node *> callee_basic_blocks;
          bool changed;

          // Checks whether a node with key [%callee entry block, %caller basic
          // block], i.e. an entry block with callstring of caller basic block,
          // exists. If not, all nodes with their corrosponding keys are
          // initilized for the callee function.
          if (nodes.find(callee_element) == nodes.end()) {
            // Check if abstract_state of call.bb is bottom or not
            dbgs(3) << "    No information regarding function call %"
                    << call->getCalledFunction()->getName() << "\n";

            callee_basic_blocks =
                register_function(callee_func, {}, callstack_depth, nodes);

            nodes[callee_element].state =
                AbstractState{callee_func, state_new, call};
            changed = true;
          } else {
            // update callee
            AbstractState before = nodes[callee_element].state;

            // Collect all basic blocks of callee_func
            for (po_iterator<BasicBlock const *>
                     I = po_begin(&callee_func->getEntryBlock()),
                     IE = po_end(&callee_func->getEntryBlock());
                 I != IE; ++I) {
              BasicBlock const *basic_block = *I;
              NodeKey key = {new_callstring, basic_block};
              callee_basic_blocks.push_back(&nodes[key]);
            }

            AbstractState state_update{callee_func, state_new, call};
            changed = nodes[callee_element].state.merge(merge_op, state_update);
          }

          // Getting the last block
          BasicBlock const *end_block = &*std::prev(callee_func->end());
          NodeKey end_element = {new_callstring, end_block};
          state_new.applyCallInst(inst, end_block, nodes[end_element].state);

          // If input parameters have changed, we want to interpret the function
          // once again and reevaluate the nodes of possible callers.
          if (changed) {
            for (auto &[key, value] : nodes) {
              if (key.second == node.basic_block and
                  not value.update_scheduled) {
                dbgs(3) << "      Adding possible caller " << key
                        << " to worklist\n";
                worklist.push_back(&value);
                value.update_scheduled = true;
              }
            }

            // Checks if the key of the callee functions entry node is already
            // on the worklist, this is necessary for recursions.
            for (Node *elem : callee_basic_blocks) {
              if (!elem->update_scheduled) {
                worklist.push_back(elem);
                elem->update_scheduled = true;

                dbgs(3) << "      Adding callee " << *elem->basic_block << " "
                        << elem->callstring << " to worklist\n";
              } else {
                dbgs(3)
                    << "      Callee already on worklist, nothing to add...\n";
              }
            }
          }
        } else {
          if (state_new.checkOperandsForBottom(inst))
            continue;
          state_new.applyDefault(inst);
        }
      }
    }

    // Merge the state back into the node
    dbgs(3) << "  Merging with stored state\n";
    bool changed = node.state.merge(merge_op, state_new);

    dbgs(2) << "  Outgoing state " << (changed ? "changed" : "didn't change")
            << ":\n";
    state_new.printOutgoing(*node.basic_block, dbgs(2), 4);

    // No changes, so no need to do anything else
    if (not changed)
      continue;

    dbgs(2) << "  State changed, notifying " << succ_size(node.basic_block)
            << (succ_size(node.basic_block) != 1 ? " successors\n"
                                                 : " successor\n");

    // Something changed and we will need to update the successors
    for (BasicBlock const *succ_bb : successors(node.basic_block)) {
      NodeKey succ_key = {{node.callstring}, succ_bb};
      Node &succ = nodes[succ_key];
      if (not succ.update_scheduled) {
        worklist.push_back(&succ);
        succ.update_scheduled = true;

        dbgs(3) << "    Adding " << succ_key << " to worklist\n";
      }
    }
  }

  if (!worklist.empty()) {
    dbgs(0) << "Iteration terminated due to exceeding loop count.\n";
  }

  std::ofstream Res(OutputFilename.c_str());
  unsigned int y = 0;
  if (Res.good()) {
    dbgs(0) << "Output filename: " << OutputFilename.c_str();

    std::error_code ErrInfo;
    llvm::raw_fd_ostream Result(OutputFilename.c_str(), ErrInfo,
                                llvm::sys::fs::F_None);
    StringRef sofar("");
    Result << "{\n";
    for (auto const &[key, node] : nodes) {
      StringRef name = (*node.basic_block).getParent()->getName();

      if (!sofar.equals(name)) {
        if (sofar.size() != 0)
          Result << "},\n";
        Result << "\"" << name << "\":{\n";
        sofar = name;
      }
      Result << "  \"" << (*node.basic_block).getName() << "\""
             << ":{\n";
      node.state.printOutgoing(*node.basic_block, Result, 4);
      Result << "  }";
      if (y++ != nodes.size() - 1)
        Result << ",";
      Result << "\n";
    }
    Result << "}}\n";
    Result.flush();
  }

  // Output the final result
  dbgs(0) << "\nFinal result:\n";
  for (auto const &[key, node] : nodes) {
    dbgs(0) << key << ":\n";
    node.state.printOutgoing(*node.basic_block, dbgs(0), 2);
  }

  return nodes;
}

template <typename AbstractState>
bool optimize(
    llvm::Module &M,
    const unordered_map<NodeKey, Node<AbstractState>> nodes2AbstractStateNode) {
  bool hasChanged = false;

  for (auto func = M.begin(); func != M.end(); func++) {
    for (Function::iterator bb = func->begin(), bbe = func->end(); bb != bbe;
         ++bb) {

      BasicBlock &tmpBB = *bb;
      Callstring callstring =
          callstring_for(M.getFunction(func->getName()), {}, 1);

      NodeKey key = {callstring, &tmpBB};
      Node<AbstractState> node = nodes2AbstractStateNode.at(key);

      // Collect the predecessors
      vector<AbstractState> predecessors;
      for (BasicBlock const *basic_block :
           llvm::predecessors(node.basic_block)) {
        AbstractState state_branched{
            nodes2AbstractStateNode.at({{node.callstring}, basic_block}).state};
        predecessors.push_back(state_branched);
      }

      for (Instruction &inst : *bb) {
        if (isa<PHINode>(inst)) {
          hasChanged |= node.state.applyPHINode(*bb, predecessors, inst);
        } else {
          hasChanged |= node.state.applyDefault(inst);
        }
      }
    }
  }

  return hasChanged;
} // namespace pcpo

bool AbstractInterpretationPass::runOnModule(llvm::Module &M) {
  // using AbstractState = AbstractStateValueSet<SimpleInterval>;
  //     Use either the standard fixpoint algorithm or the version with
  //     widening
  // executeFixpointAlgorithm<AbstractState>(M);
  auto analysisData = executeFixpointAlgorithm<ConstantFolding>(M);
  return optimize<ConstantFolding>(M, analysisData);

  //     executeFixpointAlgorithm<NormalizedConjunction>(M);
  //    executeFixpointAlgorithm<LinearSubspace>(M);
  //    executeFixpointAlgorithmWidening<AbstractState>(M);

  // We never change anything
  // return false;
}

void AbstractInterpretationPass::getAnalysisUsage(
    llvm::AnalysisUsage &info) const {
  info.setPreservesAll();
}

} /* end of namespace pcpo */
