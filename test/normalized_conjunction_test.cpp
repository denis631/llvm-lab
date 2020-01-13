#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <dlfcn.h>

#include "../src/normalized_conjunction.h"

using namespace pcpo;
using namespace llvm;

int main() {
    
    Value *x1 = (Value *) 1;
    Value *x2 = (Value *) 2;
    Value *x3 = (Value *) 3;
    Value *x4 = (Value *) 4;
    Value *x5 = (Value *) 5;
    Value *x6 = (Value *) 6;
    Value *x7 = (Value *) 7;
    Value *x8 = (Value *) 8;
    Value *x9 = (Value *) 9;
    Value *x10 = (Value *) 10;
    Value *x11 = (Value *) 11;
    Value *x12 = (Value *) 12;

    std::map<Value const *, NormalizedConjunction::Equality> E1 = {
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x2, 0}},
        {x3, {x3, 1, x1, 0}},
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x1, 15}},
        {x6, {x6, 1, x1, 3}},
        {x7, {x7, 1, x1, 2}},
        {x8, {x8, 7, x1, 15}},
        {x9, {x9, 1, nullptr, 0}},
        {x10, {x10, 1, nullptr, 2}},
        {x11, {x11, 1, nullptr, 1}},
        {x12, {x12, 1, nullptr, 3}}
    };

    std::map<Value const *, NormalizedConjunction::Equality> E2 = {
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x2, 0}},
        {x3, {x3, 1, x2, -5}},
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x2, 0}},
        {x6, {x6, 1, x2, 1}},
        {x7, {x7, 1, x2, 0}},
        {x8, {x8, 21, x2, -20}},
        {x9, {x9, 1, nullptr, 1}},
        {x10, {x10, 1, nullptr, 4}},
        {x11, {x11, 2, x1, -3}},
        {x12, {x12, 4, x1, -5}}
    };
    
    std::map<Value const*, NormalizedConjunction::Equality> expected = {
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x3, 15}},
        {x7, {x7, 1, x6, -1}},
        {x10, {x10, 2, x9, 2}},
        {x12, {x12, 2, x11, 1}}
    };
    
    NormalizedConjunction result = NormalizedConjunction::leastUpperBound(NormalizedConjunction(E1), NormalizedConjunction(E2));
    
    return result.equalaties == expected;
}
