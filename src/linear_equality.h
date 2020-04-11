#pragma once

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Instructions.h>

#include "global.h"

namespace pcpo {

class LinearEquality {
    
public:
    LinearEquality() = default;
    LinearEquality(LinearEquality const&) = default;
    LinearEquality(llvm::Value const* y);
    LinearEquality(llvm::Value const* y, int64_t a, llvm::Value const* x, int64_t b);
    LinearEquality(llvm::ConstantInt const* y);
    // y = a * x + b
    llvm::Value const* y;
    // APInt would be nicer, but our anlysis doesnt care about bit width
    int64_t a;
    llvm::Value const* x;
    int64_t b;
    
    inline bool operator<(LinearEquality const& rhs) const {
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
    
    inline bool operator>(LinearEquality const& rhs) const {
        return *this < rhs;
    };
    
    inline bool operator==(LinearEquality const& rhs) const {
        return y == rhs.y && a == rhs.a && x == rhs.x && b == rhs.b;
    };
    
    inline bool operator!=(LinearEquality const& rhs) const {
        return !(*this == rhs);
    };
    
    bool isConstant() const { return x == nullptr; };
    bool isTrivial() const { return x == y; };
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, LinearEquality a);

}
