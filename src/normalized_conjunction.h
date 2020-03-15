//
//  conjunction.h
//  PAIN
//
//  Created by Tim Gymnich on 17.1.20.
//

#pragma once

#include <unordered_map>
#include <vector>
#include <set>

#include <llvm/IR/Instructions.h>

#include "global.h"
#include "linear_equality.h"

namespace pcpo {

class NormalizedConjunction {
public:
    std::unordered_map<llvm::Value const*, LinearEquality> values;
    std::set<llvm::Value const*> validVariables;
    bool isBottom = true;
    
    NormalizedConjunction() = default;
    NormalizedConjunction(NormalizedConjunction const& state) = default;
    NormalizedConjunction(std::unordered_map<llvm::Value const*, LinearEquality> const& equalaties);
    
    explicit NormalizedConjunction(llvm::Function const& f);
    /// This constructor is used to initialize the state of a function call, to which parameters are passed.
    /// This is the "enter" function as described in "Compiler Design: Analysis and Transformation"
    explicit NormalizedConjunction(llvm::Function const* callee_func, NormalizedConjunction const& state, llvm::CallInst const* call);
    
    /// Handles the evaluation of merging points
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<NormalizedConjunction> pred_values, llvm::Instruction const& phi);
    /// Handles the evaluation of function calls
    /// This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block, NormalizedConjunction const& callee_state);
    /// Handles the evaluation of return instructions
    void applyReturnInst(llvm::Instruction const& inst);
    /// Handles the evaluation of all other instructions
    void applyDefault(llvm::Instruction const& inst);
    bool merge(Merge_op::Type op, NormalizedConjunction const& other);
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) { return; };
    bool leastUpperBound(NormalizedConjunction rhs);

    bool checkOperandsForBottom(llvm::Instruction const& inst) { return false; }
    
    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation) const;
    
    // Abstract Assignments
    void linearAssignment(llvm::Value const* xi, int64_t a, llvm::Value const* xj, int64_t b);
    void nonDeterminsticAssignment(llvm::Value const* xi);

    // Operators
    LinearEquality& operator[](llvm::Value const*);
    LinearEquality& get(llvm::Value const*);
    
protected:
    // Abstract Operators
    void Add(llvm::Instruction const& inst);
    void Sub(llvm::Instruction const& inst);
    void Mul(llvm::Instruction const& inst);
    
    /// Used for debug output
    void debug_output(llvm::Instruction const& inst, std::vector<LinearEquality> operands);
    
    // Helpers
    static std::set<LinearEquality> computeX0(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2);
    static std::set<LinearEquality> computeX1(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2);
    static std::set<LinearEquality> computeX2(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2);
    static std::set<LinearEquality> computeX4(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2);
};


}
