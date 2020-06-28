#include "true_liveness.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

namespace pcpo {

void TrueLiveness::applyPHINode(llvm::BasicBlock const &bb,
                                std::vector<TrueLiveness> const &pred_values,
                                llvm::Instruction const &inst) {
  // if value is not in live set -> ignore instruction
  auto search = liveValues.find(&inst);
  if (search == liveValues.end())
    return;

  auto phi = dyn_cast<llvm::PHINode>(&inst);
  for (llvm::Value const *value : phi->incoming_values()) {
    liveValues.insert(value);
  }
}

void TrueLiveness::applyReturnInst(llvm::Instruction const &inst) {
  auto ret = dyn_cast<llvm::ReturnInst>(&inst);
  liveValues.insert(ret->getReturnValue());
}

void TrueLiveness::applyDefault(llvm::Instruction const &inst) {
  auto br = dyn_cast<llvm::BranchInst>(&inst);
  if (br != nullptr) {
    liveValues.insert(br->getOperand(0));
    return;
  }

  // if value is not in live set -> ignore instruction
  auto search = liveValues.find(&inst);
  if (search == liveValues.end())
    return;

  for (llvm::Value const *value : inst.operand_values()) {
    liveValues.insert(value);
  }
}

bool TrueLiveness::merge(Merge_op::Type op, TrueLiveness const &other) {
  std::unordered_set<llvm::Value const *> liveValuesUnion;
  liveValuesUnion.insert(liveValues.begin(), liveValues.end());
  liveValuesUnion.insert(other.liveValues.begin(), other.liveValues.end());

  bool hasChanged = liveValues != liveValuesUnion;
  liveValues = liveValuesUnion;
  isBottom &= other.isBottom;

  return hasChanged;
}

void TrueLiveness::printOutgoing(llvm::BasicBlock const &bb,
                                 llvm::raw_ostream &out,
                                 int indentation) const {
  out << "live values:\n";
  for (auto val : liveValues) {

    auto constVal = dyn_cast<llvm::ConstantData>(val);
    if (constVal == nullptr) {
      out << '%' << val->getName() << '\n';
    }
  }
}

// transforms

bool TrueLiveness::transformPHINode(
    llvm::BasicBlock &bb, std::vector<TrueLiveness> const &pred_values,
    llvm::Instruction &inst) {

  auto search = liveValues.find(&inst);
  if (search != liveValues.end())
    return false;

  inst.dropAllReferences();
  instructionsToErase.push_back(&inst);

  return true;
}

bool TrueLiveness::transformDefault(llvm::Instruction &inst) {
  auto ret = dyn_cast<llvm::ReturnInst>(&inst);
  auto br = dyn_cast<llvm::BranchInst>(&inst);

  if (ret != nullptr || br != nullptr)
    return false;

  auto search = liveValues.find(&inst);
  if (search != liveValues.end())
    return false;

  inst.dropAllReferences();
  instructionsToErase.push_back(&inst);

  return true;
}

// void TrueLiveness::transform() {
//   dbgs(3) << "Deleting: \n";

//   for (auto inst : instructionsToErase) {

//     dbgs(3) << *inst << '\n';

//     inst->eraseFromParent();
//   }
// }

} // namespace pcpo
