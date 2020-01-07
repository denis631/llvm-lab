#pragma once

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>

#include "global.h"
#include <map>

namespace pcpo {

class NormalizedConjunction {

    public:
        enum State: char {
            INVALID, BOTTOM = 1, NORMAL = 2, TOP = 4
        };
        // TODO: consider making this a computed value based on the equalaties
        char state;
        struct Equality {
            // y = a * x + b
            llvm::Value const* y;
            // APInt would be nicer, but our anlysis doesnt care about bit width
            int64_t a;
            llvm::Value const* x;
            int64_t b;
            
            inline bool operator<(Equality const& rhs) const {
                return y < rhs.y;
            };
            
            inline bool operator>(Equality const& rhs) const {
                return y > rhs.y;
            };
            
            inline bool operator==(Equality const& rhs) const {
                return y == rhs.y && a == rhs.a && x == rhs.x && b == rhs.b;
            };
            
            bool isConstant() const { return x == nullptr; };
        };
        std::map<llvm::Value const*, Equality> equalaties;

        // AbstractDomain interface
        NormalizedConjunction(bool isTop = false): state{isTop ? TOP : BOTTOM} {}
        NormalizedConjunction(llvm::Constant const& constant);
        static NormalizedConjunction interpret(
            llvm::Instruction const& inst, std::vector<NormalizedConjunction> const& operands
        );
        static NormalizedConjunction refineBranch(
            llvm::CmpInst::Predicate pred, llvm::Value const& lhs, llvm::Value const& rhs,
            NormalizedConjunction a, NormalizedConjunction b
        );
        static NormalizedConjunction merge(Merge_op::Type op, NormalizedConjunction a, NormalizedConjunction b);

        // utils
        bool isTop() const { return state == TOP; }; /* return equalities.empty() */
        bool isBottom() const { return state == BOTTOM; }; /* never? */
        bool isNormalized() const;
    
        bool operator==(NormalizedConjunction other) const;
        bool operator!=(NormalizedConjunction other) const {return !(*this == other);}
    
    private:
        NormalizedConjunction(std::map<llvm::Value const*, Equality> equalaties);

        static NormalizedConjunction leastUpperBound(NormalizedConjunction E1, NormalizedConjunction E2);
        static NormalizedConjunction nonDeterminsticAssignment(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
        static NormalizedConjunction Add(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
        static NormalizedConjunction Sub(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
        static NormalizedConjunction Mul(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, NormalizedConjunction a);

}
