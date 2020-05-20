#include "constant_folding.h"

#include "llvm/IR/CFG.h"

using namespace llvm;

namespace pcpo {

void ConstantFolding::applyPHINode(
    llvm::BasicBlock const &bb, std::vector<ConstantFolding> const &pred_values,
    llvm::Instruction const &inst) {
  PHINode const *phiNode = dyn_cast<PHINode>(&inst);
  int i = 0;

  for (BasicBlock const *pred_bb : llvm::predecessors(&bb)) {
    auto &incoming_value = *phiNode->getIncomingValueForBlock(pred_bb);
    auto &incoming_state = pred_values[i];

    std::optional<int64_t> val = std::nullopt;

    if (llvm::ConstantInt const *c =
            llvm::dyn_cast<llvm::ConstantInt>(&incoming_value)) {
      val = std::optional{c->getSExtValue()};
    } else {
      if (incoming_state.valueToIntMapping.count(&incoming_value)) {
        val = std::optional{
            incoming_state.valueToIntMapping.find(&incoming_value)->second};
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

  switch (inst.getOpcode()) {
  case Instruction::Add:
  case Instruction::Mul:
  case Instruction::Sub:
  case Instruction::ICmp:
    break;
  default:
    return;
  }

  // We only deal with integer types
  IntegerType const *type = dyn_cast<IntegerType>(inst.getType());
  if (not type)
    return;

  // every operand should either be:
  // - in valid variables and in the hashmap to consts or
  // - int

  auto op1 = inst.getOperand(0);
  auto op2 = inst.getOperand(1);

  int intOp1 = 0;
  int intOp2 = 0;

  if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
    intOp1 = dyn_cast<ConstantInt>(op1)->getSExtValue();
    intOp2 = dyn_cast<ConstantInt>(op2)->getSExtValue();
  } else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
    auto op2Val = dyn_cast<Value>(op2);
    if (!valueToIntMapping.count(op2Val)) {
      return;
    }

    intOp1 = dyn_cast<ConstantInt>(op1)->getSExtValue();
    intOp2 = valueToIntMapping[op2Val];

    // op->setOperand(1, ConstantInt::get(op2Val->getType(), intOp2));
  } else if (isa<Value>(op1) && isa<ConstantInt>(op2)) {
    auto op1Val = dyn_cast<Value>(op1);
    if (!valueToIntMapping.count(op1Val))
      return;

    intOp1 = valueToIntMapping[op1Val];
    intOp2 = dyn_cast<ConstantInt>(op2)->getSExtValue();

    // op->setOperand(0, ConstantInt::get(op1Val->getType(), intOp1));
  } else {
    auto op1Val = dyn_cast<Value>(op1);
    auto op2Val = dyn_cast<Value>(op2);
    if (!valueToIntMapping.count(op1Val) || !valueToIntMapping.count(op2Val))
      return;

    intOp1 = valueToIntMapping[op1Val];
    intOp2 = valueToIntMapping[op2Val];
  }

  auto result = apply(inst.getOpcode(), intOp1, intOp2);
  valueToIntMapping[&inst] = result;
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

    std::unordered_map<Value const *, int> newValueToIntMapping;
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
