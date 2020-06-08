#pragma once

#include "llvm/IR/CFG.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <llvm/IR/Instructions.h>

#include "global.h"
#include "value_set.h"

namespace pcpo {

// TODO: make it generic for any domain T
template <typename T> class ConstantFolding : public AbstractStateValueSet<T> {
public:
  using AbstractStateValueSet<T>::AbstractStateValueSet;
  using AbstractStateValueSet<T>::values;

  void applyPHINode(llvm::BasicBlock const &bb,
                    std::vector<ConstantFolding> const &pred_values,
                    llvm::Instruction const &inst) {

    std::vector<AbstractStateValueSet<T>> preds;
    for (auto &x : pred_values) {
      preds.push_back(x);
    }

    AbstractStateValueSet<T>::applyPHINode(bb, preds, inst);
  }

  bool transformPHINode(llvm::BasicBlock const &bb,
                        std::vector<ConstantFolding> const &pred_values,
                        llvm::Instruction &inst) {

    llvm::PHINode *phiNode = dyn_cast<llvm::PHINode>(&inst);
    bool hasChanged = false;

    int i = 0;
    for (llvm::BasicBlock const *predBB : llvm::predecessors(&bb)) {
      auto &incomingValue = *phiNode->getIncomingValueForBlock(predBB);
      auto &incomingState = pred_values[i];

      if (values.count(&incomingValue)) {
        auto x = values.at(&incomingValue).toConstant(incomingValue.getType());

        if (x.has_value()) {
          phiNode->setIncomingValueForBlock(predBB, x.value());
          hasChanged = true;
        }
      }

      i++;
    }

    return hasChanged;
  }

  bool transformDefault(llvm::Instruction &inst) {
    bool hasChanged = false;

    for (int i = 0; i < inst.getNumOperands(); i++) {
      auto operand = inst.getOperand(i);

      if (values.count(operand)) {
        auto opConstant = values.at(operand).toConstant(operand->getType());

        if (opConstant.has_value()) {
          inst.setOperand(i, opConstant.value());
          hasChanged = true;
        }
      }
    }

    return hasChanged;
  }
};

} // namespace pcpo
