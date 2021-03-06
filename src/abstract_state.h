#pragma once

#include "llvm/IR/CFG.h"

namespace pcpo {

class AbstractStateDummy {
public:
    // This has to initialise the state to bottom.
    AbstractStateDummy() = default;

    // Creates a copy of the state. Using the default copy-constructor should be fine here, but if
    // some members do something weird you maybe want to implement this as
    //     AbstractState().merge(state)
    AbstractStateDummy(AbstractStateDummy const& state) = default;

    // Initialise the state to the incoming state of the function. This should do something like
    // assuming the parameters can be anything.
    explicit AbstractStateDummy(llvm::Function const& f) {}

    // Initialise the state of a function call with parameters of the caller.
    // This is the "enter" function as described in "Compiler Design: Analysis and Transformation"
    explicit AbstractStateDummy(llvm::Function const* callee_func, AbstractStateDummy const& state,
                                llvm::CallInst const* call) {}

    // Apply functions apply the changes needed to reflect executing the instructions in the basic block. Before
    // this operation is called, the state is the one upon entering bb, afterwards it should be (an
    // upper bound of) the state leaving the basic block.
    //  predecessors contains the outgoing state for all the predecessors, in the same order as they
    // are listed in llvm::predecessors(bb).

    // Applies instructions within the PHI node, needed for merging
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<AbstractStateDummy> const& pred_values,
                      llvm::Instruction const& inst) {};

    // This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block,
                       AbstractStateDummy const& callee_state) {};

    // Evaluates return instructions, needed for the main function and the debug output
    void applyReturnInst(llvm::Instruction const& inst) {};

    // Handles all cases different from the three above
    void applyDefault(llvm::Instruction const& inst) {};

    // This 'merges' two states, which is the operation we do fixpoint iteration over. Currently,
    // there are three possibilities for op:
    //   1. UPPER_BOUND: This has to return some upper bound of itself and other, with more precise
    //      bounds being preferred.
    //   2. WIDEN: Same as UPPER_BOUND, but this operation should sacrifice precision to converge
    //      quickly. Returning T would be fine, though maybe not optimal. For example for intervals,
    //      an implementation could ensure to double the size of the interval.
    //   3. NARROW: Return a value between the intersection of the state and other, and the
    //      state. In pseudocode:
    //          intersect(state, other) <= narrow(state, other) <= state
    // For all of the above, this operation returns whether the state changed as a result.
    // IMPORTANT: The simple fixpoint algorithm only performs UPPER_BOUND, so you do not need to
    // implement the others if you just use that one. (The more advanced algorithm in
    // fixpoint_widening.cpp uses all three operations.)
    bool merge(Merge_op::Type op, AbstractStateDummy const& other) { return false; };

    // Restrict the set of values to the one that allows 'from' to branch towards
    // 'towards'. Starting with the state when exiting from, this should compute (an upper bound of)
    // the possible values that would reach the block towards. Doing nothing thus is a valid
    // implementation.
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) {};

    // Functions to generate the debug output. printIncoming should output the state as of entering
    // the basic block, printOutcoming the state when leaving it.
    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
};

}
