#pragma once

#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>

#include <unordered_set>

#include "global.h"

namespace pcpo {

class TrueLiveness {
public:
  // This has to initialise the state to bottom.
  TrueLiveness(bool isTop = false) { isBottom = not isTop; };

  // Creates a copy of the state. Using the default copy-constructor should be
  // fine here, but if some members do something weird you maybe want to
  // implement this as
  //     AbstractState().merge(state)
  TrueLiveness(TrueLiveness const &state) = default;

  // Initialise the state to the incoming state of the function. This should do
  // something like assuming the parameters can be anything.
  explicit TrueLiveness(llvm::Function const &f) : TrueLiveness(true) {}

  // Initialise the state of a function call with parameters of the caller.
  // This is the "enter" function as described in "Compiler Design: Analysis and
  // Transformation"
  explicit TrueLiveness(llvm::Function const *callee_func,
                        TrueLiveness const &state, llvm::CallInst const *call)
      : TrueLiveness(true) {}

  // Apply functions apply the changes needed to reflect executing the
  // instructions in the basic block. Before this operation is called, the state
  // is the one upon entering bb, afterwards it should be (an upper bound of)
  // the state leaving the basic block.
  //  predecessors contains the outgoing state for all the predecessors, in the
  //  same order as they
  // are listed in llvm::predecessors(bb).

  // Applies instructions within the PHI node, needed for merging
  void applyPHINode(llvm::BasicBlock const &bb,
                    std::vector<TrueLiveness> const &pred_values,
                    llvm::Instruction const &inst);

  // This is the "combine" function as described in "Compiler Design: Analysis
  // and Transformation"
  void applyCallInst(llvm::Instruction const &inst,
                     llvm::BasicBlock const *end_block,
                     TrueLiveness const &callee_state){};

  void applyReturnInst(llvm::Instruction const &inst);

  void applyDefault(llvm::Instruction const &inst);

  bool merge(Merge_op::Type op, TrueLiveness const &other);

  void branch(llvm::BasicBlock const &from, llvm::BasicBlock const &towards){};

  bool checkOperandsForBottom(llvm::Instruction const &inst) { return false; };

  void printIncoming(llvm::BasicBlock const &bb, llvm::raw_ostream &out,
                     int indentation = 0) const {};
  void printOutgoing(llvm::BasicBlock const &bb, llvm::raw_ostream &out,
                     int indentation = 0) const;

  // transformation
  bool transformPHINode(llvm::BasicBlock &bb,
                        std::vector<TrueLiveness> const &pred_values,
                        llvm::Instruction &inst);

  bool transformDefault(llvm::Instruction &inst);
  void transform();

  std::unordered_set<llvm::Value const *> liveValues;
  std::vector<llvm::Instruction *> instructionsToErase;
  bool isBottom = true;
};

} // namespace pcpo
