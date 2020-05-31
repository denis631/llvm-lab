#include "constant_folding.h"

#include "llvm/IR/CFG.h"

using namespace llvm;

namespace pcpo {

void ConstantFolding::applyPHINode(
    llvm::BasicBlock const &bb, std::vector<ConstantFolding> const &pred_values,
    llvm::Instruction const &inst) {
  PHINode const *phiNode = dyn_cast<PHINode>(&inst);
  int i = 0;

  for (BasicBlock const *predBB : llvm::predecessors(&bb)) {
    auto &incomingValue = *phiNode->getIncomingValueForBlock(predBB);
    auto &incomingState = pred_values[i];

    std::optional<uint64_t> val = std::nullopt;

    if (llvm::ConstantInt const *c =
            llvm::dyn_cast<llvm::ConstantInt>(&incomingValue)) {
      val = std::optional{c->getZExtValue()};
    } else {
      if (incomingState.valueToIntMapping.count(&incomingValue)) {
        val = std::optional{
            incomingState.valueToIntMapping.find(&incomingValue)->second};
      }
    }

    if (valueToIntMapping.count(&inst)) {
      if (!val.has_value()) {
        valueToIntMapping.erase(&inst);
        return;
      }

      auto acc = *this;
      acc.valueToIntMapping[&inst] = val.value();
      merge(Merge_op::UPPER_BOUND, acc);
    } else {
      if (!val.has_value()) {
        continue;
      }

      valueToIntMapping[&inst] = val.value();
    }

    i++;
  }
}

bool ConstantFolding::applyPHINode(
    llvm::BasicBlock const &bb, std::vector<ConstantFolding> const &pred_values,
    llvm::Instruction &inst) {

  PHINode *phiNode = dyn_cast<PHINode>(&inst);
  bool hasChanged = false;
  int i = 0;

  for (BasicBlock const *predBB : llvm::predecessors(&bb)) {
    auto &incomingValue = *phiNode->getIncomingValueForBlock(predBB);
    auto &incomingState = pred_values[i];

    if (incomingState.valueToIntMapping.count(&incomingValue)) {
      auto value = incomingState.valueToIntMapping.at(&incomingValue);

      phiNode->setIncomingValueForBlock(
          predBB, ConstantInt::get(incomingValue.getType(), value));

      hasChanged = true;
    }

    i++;
  }

  return hasChanged;
} // namespace pcpo

bool ConstantFolding::isValidDefaultOpcode(
    const llvm::Instruction &inst) const {
  switch (inst.getOpcode()) {
  case Instruction::Add:
  case Instruction::Mul:
  case Instruction::Sub:
  case Instruction::ICmp:
  case Instruction::Br:
    return true;
  default:
    return false;
  }
}

void ConstantFolding::applyDefault(const llvm::Instruction &inst) {
  auto apply = [](unsigned int opCode, int a, int b) {
    switch (opCode) {
    case Instruction::Add:
      return a + b;
    case Instruction::Mul:
      return a * b;
    case Instruction::Sub:
      return a - b;
    case Instruction::ICmp:
      return int(a == b);
    default:
      return -1;
    }
  };

  auto toInt = [this](Value *val) -> std::optional<uint64_t> {
    // every operand should either be:
    // - in the hashmap to consts or
    // - int

    if (isa<ConstantInt>(val)) {
      return std::optional{dyn_cast<ConstantInt>(val)->getZExtValue()};
    } else if (valueToIntMapping.count(val)) {
      return std::optional{valueToIntMapping[val]};
    } else {
      return std::nullopt;
    }
  };

  if (!isValidDefaultOpcode(inst))
    return;

  // We only deal with integer types
  IntegerType const *type = dyn_cast<IntegerType>(inst.getType());
  if (not type)
    return;

  // only binops
  if (inst.getNumOperands() != 2)
    return;

  auto intOp1 = toInt(inst.getOperand(0));
  auto intOp2 = toInt(inst.getOperand(1));

  if (!intOp1.has_value() || !intOp2.has_value()) {
    return;
  }

  auto result = apply(inst.getOpcode(), intOp1.value(), intOp2.value());
  valueToIntMapping[&inst] = result;
}

bool ConstantFolding::applyDefault(llvm::Instruction &inst) {
  if (!isValidDefaultOpcode(inst))
    return false;

  bool hasChanged = false;

  for (int i = 0; i < inst.getNumOperands(); i++) {
    auto operand = inst.getOperand(i);

    if (valueToIntMapping.count(operand)) {
      inst.setOperand(
          i, ConstantInt::get(operand->getType(), valueToIntMapping[operand]));
      hasChanged = true;
    }
  }

  return hasChanged;
}

bool ConstantFolding::merge(Merge_op::Type op, ConstantFolding const &other) {
  if (isBottom && other.isBottom) {
    return false;
  } else if (isBottom && !other.isBottom) {
    valueToIntMapping = other.valueToIntMapping;
    isBottom = false;
    return true;
  } else if (!isBottom && other.isBottom) {
    return false;
  } else if (!isBottom && !other.isBottom) {
    if (op != Merge_op::UPPER_BOUND)
      return false;

    std::unordered_map<Value const *, uint64_t> newValueToIntMapping;
    std::unordered_set<Value const *> values;

    for (auto kv : valueToIntMapping) {
      values.insert(kv.first);
    }
    for (auto kv : other.valueToIntMapping) {
      values.insert(kv.first);
    }

    for (auto val : values) {
      if (valueToIntMapping.count(val) && other.valueToIntMapping.count(val)) {
        auto a = valueToIntMapping.at(val);
        auto b = other.valueToIntMapping.at(val);

        if (a == b) {
          newValueToIntMapping[val] = a;
        }
      }
    }

    bool hasChanged = valueToIntMapping == newValueToIntMapping;
    valueToIntMapping = newValueToIntMapping;
    return !hasChanged;
  }
}

void ConstantFolding::printOutgoing(const llvm::BasicBlock &bb,
                                    llvm::raw_ostream &out,
                                    int indentation = 0) const {
  for (const auto &[key, value] : valueToIntMapping) {
    out << '%' << key->getName() << " = " << value << '\n';
  }
}

} // namespace pcpo
