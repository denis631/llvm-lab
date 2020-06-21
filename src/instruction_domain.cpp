#include "instruction_domain.h"

namespace pcpo {

bool InstructionDomain::operator==(InstructionDomain o) const {
  if (isBottom() && o.isBottom())
    return true;

  if (inst.has_value() && o.inst.has_value()) {
    return inst.value()->isIdenticalTo(o.inst.value());
  }

  return false;
}

InstructionDomain InstructionDomain::refineBranch(llvm::CmpInst::Predicate pred,
                                                  llvm::Value const &a_value,
                                                  llvm::Value const &b_value,
                                                  InstructionDomain a,
                                                  InstructionDomain b) {
  return a;
}

InstructionDomain InstructionDomain::merge(Merge_op::Type op,
                                           InstructionDomain a,
                                           InstructionDomain b) {
  if (op != Merge_op::UPPER_BOUND)
    return InstructionDomain(true);

  if (a.isBottom()) {
    return b;
  } else if (b.isBottom()) {
    return a;
  } else {
    if (a != b)
      return InstructionDomain(true);

    // TODO: alternative to comes before?
    // return inst.value()->comesBefore(o.inst.value());
    return a;
  }
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const InstructionDomain instructionDomain) {

  if (instructionDomain.isBottom()) {
    os << "⊥";
  } else {
    if (instructionDomain.inst.has_value()) {
      os << instructionDomain.inst.value();
    } else {
      os << "⊤";
    }
  }

  return os;
}

} // namespace pcpo
