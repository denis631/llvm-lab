#pragma once

#include "llvm/IR/CFG.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <llvm/IR/Instructions.h>

#include "global.h"
#include "instruction_domain.h"
#include "value_set.h"

namespace pcpo {
class RemovingSuperfluousComputations
    : public AbstractStateValueSet<InstructionDomain> {
public:
  using AbstractStateValueSet<InstructionDomain>::AbstractStateValueSet;
  using AbstractStateValueSet<InstructionDomain>::values;

  void
  applyPHINode(llvm::BasicBlock const &bb,
               std::vector<RemovingSuperfluousComputations> const &pred_values,
               llvm::Instruction const &inst) {

    std::vector<AbstractStateValueSet<InstructionDomain>> preds;
    for (auto &x : pred_values) {
      preds.push_back(x);
    }

    AbstractStateValueSet<InstructionDomain>::applyPHINode(bb, preds, inst);
  }

  bool transformPHINode(
      llvm::BasicBlock const &bb,
      std::vector<RemovingSuperfluousComputations> const &pred_values,
      llvm::Instruction &inst) {

    return false;
  }

  bool transformDefault(llvm::Instruction &inst,
                        const std::unordered_set<llvm::Value *> seen) {
    std::optional<llvm::Value *> lookedValue = std::nullopt;

    for (const auto [val, instDomain] : values) {
      if (instDomain.inst.has_value() &&
          instDomain.inst.value()->isIdenticalTo(&inst) &&
          instDomain.inst.value()->getValueName() != inst.getValueName()) {

        for (auto it = seen.begin(); it != seen.end(); it++) {
          if (val == *it) {
            lookedValue = std::optional{*it};
            break;
          }
        }
      }
    }

    if (!lookedValue.has_value()) {
      return false;
    }

    // if there is another instruction that covers this computation -> erase
    // it from everywhere:
    // - from available expression mapping and
    // - from basic block and replace all its usages
    if (values.count(&inst)) {
      values.erase(&inst);
    }

    inst.replaceAllUsesWith(lookedValue.value());

    return true;
  }
};
} // namespace pcpo
