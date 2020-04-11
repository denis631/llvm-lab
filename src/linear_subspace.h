#pragma once

#include <llvm/IR/Instructions.h>

#include "global.h"
#include "simple_matrix.h"
#include "sparse_matrix.h"

#include <unordered_map>

namespace pcpo {

class LinearSubspace {
private:
    /// Only valid when `createVariableIndexMap` has been generated.
    int getNumberOfVariables() const { return index.size(); };
    std::unordered_map<llvm::Value const*, int> createVariableIndexMap(llvm::Function const& func);
public:
    /// Type used for Matrix values.
    using T = double;
    using MatrixType = SparseMatrix<T>;

    std::unordered_map<llvm::Value const*, int> index;
    std::vector<MatrixType> basis;
    bool isBottom = true;
    int getWidth() { return index.size() + 1; };
    int getHeight() { return index.size() + 1; };

    LinearSubspace() = default;
    LinearSubspace(LinearSubspace const& state) = default;
    virtual ~LinearSubspace() = default;

    explicit LinearSubspace(llvm::Function const& func);
    /// This constructor is used to initialize the state of a function call, to which parameters are passed.
    /// This is the "enter" function as described in "Compiler Design: Analysis and Transformation"
    explicit LinearSubspace(llvm::Function const* callee_func, LinearSubspace const& state, llvm::CallInst const* call);

    /// Handles the evaluation of merging points
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<LinearSubspace> const& pred_values, llvm::Instruction const& phi);
    /// Handles the evaluation of function calls
    /// This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block, LinearSubspace const& callee_state);
    /// Handles the evaluation of return instructions
    void applyReturnInst(llvm::Instruction const& inst);
    /// Handles the evaluation of all other instructions
    void applyDefault(llvm::Instruction const& inst);
    bool merge(Merge_op::Type op, LinearSubspace const& other);
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) { return; };
    bool leastUpperBound(LinearSubspace const& rhs);

    bool checkOperandsForBottom(llvm::Instruction const& inst) { return false; }

    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;

    // Abstract Assignments
    void affineAssignment(llvm::Value const* xi, T a, llvm::Value const* xj, T b);
    void affineAssignment(llvm::Value const* xi, std::unordered_map<llvm::Value const*,T> relations, T constant);
    void nonDeterminsticAssignment(llvm::Value const* xi);

    virtual void print() const;

protected:
    // Abstract Operators
    void Add(llvm::Instruction const& inst);
    void Sub(llvm::Instruction const& inst);
    void Mul(llvm::Instruction const& inst);

    /// Used for debug output
    void debug_output(llvm::Instruction const& inst, MatrixType operands);

    MatrixType createTransformationMatrix(llvm::Instruction const& inst);
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, LinearSubspace const& relation);


}
