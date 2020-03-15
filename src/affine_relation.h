#pragma once

#include <llvm/IR/Instructions.h>

#include "global.h"
#include "matrix.h"

#include <unordered_map>

namespace pcpo {

class AffineRelation {
private:
    /// Only valid when `createVariableIndexMap` has been generated.
    int getNumberOfVariables() const { return index.size(); };
    std::unordered_map<llvm::Value const*, int> createVariableIndexMap(llvm::Function const& func);
public:
    std::unordered_map<llvm::Value const*, int> index;
    std::vector<Matrix<int>> basis;
    bool isBottom = true;

    AffineRelation() = default;
    AffineRelation(AffineRelation const& state) = default;

    explicit AffineRelation(llvm::Function const& func);
    /// This constructor is used to initialize the state of a function call, to which parameters are passed.
    /// This is the "enter" function as described in "Compiler Design: Analysis and Transformation"
    explicit AffineRelation(llvm::Function const* callee_func, AffineRelation const& state, llvm::CallInst const* call);

    /// Handles the evaluation of merging points
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<AffineRelation> pred_values, llvm::Instruction const& phi);
    /// Handles the evaluation of function calls
    /// This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block, AffineRelation const& callee_state);
    /// Handles the evaluation of return instructions
    void applyReturnInst(llvm::Instruction const& inst);
    /// Handles the evaluation of all other instructions
    void applyDefault(llvm::Instruction const& inst);
    bool merge(Merge_op::Type op, AffineRelation const& other);
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) { return; };
    bool leastUpperBound(AffineRelation rhs);

    bool checkOperandsForBottom(llvm::Instruction const& inst) { return false; }

//    Matrix<int> AffineRelation::getAbstractValue(llvm::Value const& value) const;

    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;

    // Abstract Assignments
    void affineAssignment(llvm::Value const* xi, int64_t a, llvm::Value const* xj, int64_t b);
    void affineAssignment(llvm::Value const* xi, std::unordered_map<llvm::Value const*,int> relations, int constant);
    void nonDeterminsticAssignment(llvm::Value const* xi);

protected:
    // Abstract Operators
    void Add(llvm::Instruction const& inst);
    void Sub(llvm::Instruction const& inst);
    void Mul(llvm::Instruction const& inst);

    /// Used for debug output
    void debug_output(llvm::Instruction const& inst, Matrix<int> operands);

    Matrix<int> createTransformationMatrix(llvm::Instruction const& inst);
};


}
