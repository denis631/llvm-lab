#pragma once

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"

#include "global.h"

namespace pcpo {
class InstructionDomain {
public:
  // This has to initialise to either top or bottom, depending on the flags
  InstructionDomain(bool isTop = false) { bottom = not isTop; };

  // Copy constructor. We need to be able to copy this type. (To be precise, we
  // also need copy assignment.) Probably you can just ignore this and leave the
  // default, compiler-generated copy constructor.
  InstructionDomain(InstructionDomain const &) = default;

  // Initialise from a constant
  InstructionDomain(llvm::Constant const &constant) : InstructionDomain(true) {}

  // Initialise from an instruction
  InstructionDomain(llvm::Instruction const &instruction)
      : InstructionDomain(true) {
    inst = std::optional{&instruction};
  }

  // This method does the actual abstract interpretation by executing the
  // instruction on the abstract domain, return (an upper bound of) the result.
  // Relevant instructions are mostly the arithmetic ones (like add, sub, mul,
  // etc.). Comparisons are handled mostly using refineBranch, however you can
  // still give bounds on their results here. (They just are not as useful for
  // branching.) Control-flowy instructions, like phi nodes, are also handled
  // above.
  static InstructionDomain
  interpret(llvm::Instruction const &inst,
            std::vector<InstructionDomain> const &operands) {
    return InstructionDomain(inst);
  }

  // Return whether the two values represent the same thing
  bool operator==(InstructionDomain o) const;
  bool operator!=(InstructionDomain o) const { return not(*this == o); }

  // Refine a by using the information that the value has to fulfill the
  // predicate w.r.t. b. For example, if the domain is an interval domain:
  //     refineBranch( ULE, [5, 10], [7, 8] ) => [5, 8]
  // or
  //     refineBranch( ULE, [5, 10], [7, 8] ) => [5, 10]
  // would be valid implementations, though the former is more precise. In this
  // case the only relevant information is that the number has to be less than
  // or equal to 8.
  //  The following properties have to be fulfilled, if c = refineBranch(~, a,
  //  b):
  //     1. c <= a
  //     2. For all x in a, y in b with x ~ y we have x in c.
  // For more or less stupid reasons this function also gets the corresponding
  // llvm values.
  static InstructionDomain refineBranch(llvm::CmpInst::Predicate pred,
                                        llvm::Value const &a_value,
                                        llvm::Value const &b_value,
                                        InstructionDomain a,
                                        InstructionDomain b);

  // This mirrors the AbstractState::merge method (documented in
  // AbstractStateDummy), so please refer to that for a description of the
  // different operations. Instead of modifying the object, this returns a new
  // one containing the result of the operation.
  static InstructionDomain merge(Merge_op::Type op, InstructionDomain a,
                                 InstructionDomain b);

  bool isBottom() const { return bottom; }

  std::optional<llvm::Instruction const *> inst = std::nullopt;

private:
  bool bottom;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const InstructionDomain InstructionDomain);
} // namespace pcpo
