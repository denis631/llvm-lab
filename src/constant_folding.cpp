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

    auto val = incomingState.getIntForValue(&incomingValue);

    if (!val.has_value()) {
      valueToIntMapping.erase(&inst);
      return;
    }

    if (valueToIntMapping.count(&inst)) {
      auto prevVal = valueToIntMapping[&inst];

      if (val.value() != prevVal) {
        valueToIntMapping.erase(&inst);
        return;
      }
    }

    valueToIntMapping[&inst] = val.value();
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

    auto opValue = incomingState.getIntForValue(&incomingValue);
    if (opValue.has_value()) {

      phiNode->setIncomingValueForBlock(
          predBB, ConstantInt::get(incomingValue.getType(), opValue.value()));

      hasChanged = true;
    }

    i++;
  }

  return hasChanged;
}

void ConstantFolding::applyCallInst(llvm::Instruction const &inst,
                                    llvm::BasicBlock const *end_block,
                                    ConstantFolding const &callee_state) {
  if (callee_state.returnVal.has_value()) {
    valueToIntMapping[&inst] = callee_state.returnVal.value();
  }
}

void ConstantFolding::applyReturnInst(llvm::Instruction const &inst) {
  returnVal = getIntForValue(dyn_cast<ReturnInst>(&inst)->getReturnValue());
}

void ConstantFolding::applyDefault(const llvm::Instruction &inst) {
  auto apply = [&inst](APInt a, APInt b) {
    bool overflown = false;
    APInt res;

#define BIN_CASE(opcode, func)                                                 \
  case Instruction::BinaryOps::opcode:                                         \
    res = a.func(b, overflown);                                                \
    if (overflown) {                                                           \
      dbgs(4) << "overflow detected!\n";                                       \
    }                                                                          \
    return res;

#define CMP_CASE(pred, f)                                                      \
  case CmpInst::Predicate::pred:                                               \
    return APInt(1, a.f(b));

    switch (inst.getOpcode()) {
      BIN_CASE(Add, sadd_ov)
      BIN_CASE(Sub, ssub_ov)
      BIN_CASE(Mul, smul_ov)
      BIN_CASE(SDiv, sdiv_ov)

    case Instruction::ICmp:
      auto cmp = dyn_cast<ICmpInst>(&inst);

      switch (cmp->getPredicate()) {
        CMP_CASE(ICMP_EQ, eq)
        CMP_CASE(ICMP_NE, ne)
        CMP_CASE(ICMP_UGT, ugt)
        CMP_CASE(ICMP_UGE, uge)
        CMP_CASE(ICMP_ULT, ult)
        CMP_CASE(ICMP_ULE, ule)
        CMP_CASE(ICMP_SGT, sgt)
        CMP_CASE(ICMP_SGE, sge)
        CMP_CASE(ICMP_SLT, slt)
        CMP_CASE(ICMP_SLE, sle)
      };
    }

#undef BIN_CASE
#undef CMP_CASE

    assert(false);
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

  auto intOp1 = getIntForValue(inst.getOperand(0));
  auto intOp2 = getIntForValue(inst.getOperand(1));

  if (!intOp1.has_value() || !intOp2.has_value()) {
    return;
  }

  auto result = apply(intOp1.value(), intOp2.value());
  valueToIntMapping[&inst] = result;
}

bool ConstantFolding::applyDefault(llvm::Instruction &inst) {
  if (!isValidDefaultOpcode(inst))
    return false;

  bool hasChanged = false;

  for (int i = 0; i < inst.getNumOperands(); i++) {
    auto operand = inst.getOperand(i);
    auto opValue = getIntForValue(operand);

    if (opValue.has_value()) {
      inst.setOperand(i, ConstantInt::get(operand->getType(), opValue.value()));
      hasChanged = true;
    }
  }

  return hasChanged;
}

bool ConstantFolding::merge(Merge_op::Type op, ConstantFolding const &other) {
  auto lub = [](std::unordered_map<Value const *, APInt> a,
                std::unordered_map<Value const *, APInt> b) {
    std::unordered_set<Value const *> values;
    std::unordered_map<Value const *, APInt> result;

    for (auto kv : a) {
      values.insert(kv.first);
    }
    for (auto kv : b) {
      values.insert(kv.first);
    }

    for (auto val : values) {
      if (a.count(val) && b.count(val)) {
        auto x = a.at(val);
        auto y = b.at(val);

        if (x == y) {
          result[val] = x;
        }
      }
    }

    return result;
  };

  if (isBottom && other.isBottom) {
    // passing params from previous state
    bool hasChanged;

    if (other.justArgumentHolder) {
      if (justArgumentHolder) {
        // in case when both calls didn't execute
        // f(4,5)
        // f(5,5)
        auto tmp = lub(valueToIntMapping, other.valueToIntMapping);
        hasChanged = tmp == valueToIntMapping;
        valueToIntMapping = tmp;
      } else {
        // in case when one is a new state and the other is arg holder
        // (function call, but not executed)
        hasChanged = valueToIntMapping == other.valueToIntMapping;
        valueToIntMapping = other.valueToIntMapping;
      }
    }

    return hasChanged;
  } else if (isBottom && !other.isBottom) {
    valueToIntMapping = other.valueToIntMapping;
    returnVal = other.returnVal;
    isBottom = false;
    return true;
  } else if (!isBottom && other.isBottom) {
    return false;
  } else if (!isBottom && !other.isBottom) {
    if (op != Merge_op::UPPER_BOUND)
      return false;

    auto newValueToIntMapping = lub(valueToIntMapping, other.valueToIntMapping);
    std::optional<APInt> newReturnVal = std::nullopt;

    if (returnVal.has_value() && other.returnVal.has_value()) {
      newReturnVal = returnVal.value() == other.returnVal.value()
                         ? returnVal
                         : std::nullopt;
    }

    bool hasChanged =
        valueToIntMapping != newValueToIntMapping || returnVal != newReturnVal;

    valueToIntMapping = newValueToIntMapping;
    returnVal = newReturnVal;

    return hasChanged;
  }
} // namespace pcpo

void ConstantFolding::printVariableMappings(llvm::raw_ostream &out) const {
  out << "state is bottom ? " << (isBottom ? "YES" : "NO") << '\n';
  out << "stored var mappings:\n";

  for (const auto &[key, value] : valueToIntMapping) {
    out << key << ' ' << '%' << key->getName() << " = " << value << '\n';
  }

  out << "return = "
      << (returnVal.has_value() ? returnVal.value().toString(10, true) : "???")
      << '\n';

  out << "---\n";
}

void ConstantFolding::printIncoming(llvm::BasicBlock const &bb,
                                    llvm::raw_ostream &out,
                                    int indentation) const {
  printVariableMappings(out);
}

void ConstantFolding::printOutgoing(const llvm::BasicBlock &bb,
                                    llvm::raw_ostream &out,
                                    int indentation = 0) const {
  printVariableMappings(out);
}

bool ConstantFolding::isValidDefaultOpcode(
    const llvm::Instruction &inst) const {
  switch (inst.getOpcode()) {
  case Instruction::Add:
  case Instruction::Mul:
  case Instruction::Sub:
  case Instruction::ICmp:
  case Instruction::Br:
  case Instruction::Ret:
    return true;
  default:
    return false;
  }
}

std::optional<APInt> ConstantFolding::getIntForValue(Value const *val) const {
  // every operand should either be:
  // - int
  // - in variables map

  if (isa<ConstantInt>(val)) {
    return std::optional{dyn_cast<ConstantInt>(val)->getValue()};
  } else if (valueToIntMapping.count(val)) {
    return std::optional{valueToIntMapping.at(val)};
  } else {
    return std::nullopt;
  }
}

} // namespace pcpo
