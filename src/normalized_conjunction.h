#pragma once

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>

#include "global.h"
#include <map>
#include <set>

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
                if (y == rhs.y) {
                    if (a == rhs.a) {
                        if (x == rhs.x) {
                            if (b == rhs.b) {
                                return false;
                            } else {
                                return b < rhs.b;
                            }
                        } else {
                            return x < rhs.x;
                        }
                    } else {
                        return a < rhs.a;
                    }
                } else {
                    return y < rhs.y;
                }
            };
            
            inline bool operator>(Equality const& rhs) const {
                return *this < rhs;
            };
            
            inline bool operator==(Equality const& rhs) const {
                return y == rhs.y && a == rhs.a && x == rhs.x && b == rhs.b;
            };
            
            inline bool operator!=(Equality const& rhs) const {
                return !(*this == rhs);
            };
            
            bool isConstant() const { return x == nullptr; };
        };
        std::map<llvm::Value const*, Equality> equalaties;
        State getState() const { return equalaties.empty() ? TOP : NORMAL; };

        // AbstractDomain interface
        NormalizedConjunction(bool isTop = false): state{isTop ? TOP : BOTTOM} {}
        NormalizedConjunction(std::map<llvm::Value const*, Equality> equalaties);
        NormalizedConjunction(llvm::Constant const& constant);
        static NormalizedConjunction interpret(
            llvm::Instruction const& inst, std::vector<NormalizedConjunction> const& operands
        );
        static NormalizedConjunction refineBranch(
            llvm::CmpInst::Predicate pred, llvm::Value const& lhs, llvm::Value const& rhs,
            NormalizedConjunction a, NormalizedConjunction b
        );
        static NormalizedConjunction merge(Merge_op::Type op, NormalizedConjunction a, NormalizedConjunction b);
        static NormalizedConjunction leastUpperBound(NormalizedConjunction E1, NormalizedConjunction E2);
    
        // utils
        bool isTop() const { return getState() == TOP; }; /* return equalities.empty() */
        bool isBottom() const { return getState() == BOTTOM; }; /* never? */
        bool isNormalized() const;
    
        // TODO: [] operator ?
        bool operator==(NormalizedConjunction other) const;
        bool operator!=(NormalizedConjunction other) const {return !(*this == other);}
    
    protected:
        // Assignments
        static NormalizedConjunction linearAssignment(NormalizedConjunction E, llvm::Value const* xi, int64_t a, llvm::Value const* xj, int64_t b);
        static NormalizedConjunction nonDeterminsticAssignment(NormalizedConjunction E, llvm::Value const* xi);

        static NormalizedConjunction Add(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
        static NormalizedConjunction Sub(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
        static NormalizedConjunction Mul(llvm::Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs);
    
        // Helpers
        static std::set<Equality> computeX0(std::set<Equality> const& E1, std::set<Equality> const& E2);
        static std::set<Equality> computeX1(std::set<Equality> const& E1, std::set<Equality> const& E2);
        static std::set<Equality> computeX2(std::set<Equality> const& E1, std::set<Equality> const& E2);
        static std::set<Equality> computeX4(std::set<Equality> const& E1, std::set<Equality> const& E2);
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, NormalizedConjunction a);

}
