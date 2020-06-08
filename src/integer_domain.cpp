#include "integer_domain.h"

namespace pcpo {

IntegerDomain
IntegerDomain::interpret(llvm::Instruction const &inst,
                         std::vector<IntegerDomain> const &operands) {
  auto apply = [&inst](llvm::APInt a, llvm::APInt b) {
    bool overflown = false;
    llvm::APInt res;

#define BIN_CASE(opcode, func)                                                 \
  case llvm::Instruction::BinaryOps::opcode:                                   \
    res = a.func(b, overflown);                                                \
    if (overflown) {                                                           \
      dbgs(4) << "overflow detected!\n";                                       \
    }                                                                          \
    return IntegerDomain(res);

#define CMP_CASE(pred, f)                                                      \
  case llvm::CmpInst::Predicate::pred:                                         \
    return IntegerDomain(llvm::APInt(1, a.f(b)));

    switch (inst.getOpcode()) {
      BIN_CASE(Add, sadd_ov)
      BIN_CASE(Sub, ssub_ov)
      BIN_CASE(Mul, smul_ov)
      BIN_CASE(SDiv, sdiv_ov)

    case llvm::Instruction::ICmp:
      auto cmp = dyn_cast<llvm::ICmpInst>(&inst);

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

    return IntegerDomain(true);
  };

  // only binops
  if (operands.size() != 2)
    return IntegerDomain(true);

  auto a = operands.at(0).toInt();
  auto b = operands.at(1).toInt();

  if (!a.has_value() || !b.has_value()) {
    return IntegerDomain(true);
  }

  return apply(a.value(), b.value());
}

bool IntegerDomain::operator==(IntegerDomain o) const {
  if (isBottom() && o.isBottom())
    return true;

  auto a = toInt();
  auto b = o.toInt();

  if (a.has_value() && b.has_value()) {
    return a.value() == b.value();
  }

  return false;
}

IntegerDomain IntegerDomain::refineBranch(llvm::CmpInst::Predicate pred,
                                          llvm::Value const &a_value,
                                          llvm::Value const &b_value,
                                          IntegerDomain a, IntegerDomain b) {
  return a;
}

IntegerDomain IntegerDomain::merge(Merge_op::Type op, IntegerDomain a,
                                   IntegerDomain b) {
  if (op != Merge_op::UPPER_BOUND)
    return IntegerDomain(true);

  if (a.isBottom()) {
    return b;
  } else if (b.isBottom()) {
    return a;
  } else {
    if (a != b)
      return IntegerDomain(true);

    return a;
  }
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const IntegerDomain integerDomain) {

  if (integerDomain.isBottom()) {
    os << "⊥";
  } else {
    os << (integerDomain.toInt().has_value()
               ? integerDomain.toInt().value().toString(10, true)
               : "⊤");
  }

  return os;
}

} // namespace pcpo
