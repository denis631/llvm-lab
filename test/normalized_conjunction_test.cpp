#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <dlfcn.h>

#include "../src/normalized_conjunction.h"

using namespace pcpo;
using namespace llvm;


class NormalizedConjunctionTest: NormalizedConjunction {

public:
    static bool runTestAll();
    static bool runTestX0();
    static bool runTestX1();
    static bool runTestX2();
    static bool runTestX4();
};

const Value *x1 = (Value *) 1;
const Value *x2 = (Value *) 2;
const Value *x3 = (Value *) 3;
const Value *x4 = (Value *) 4;
const Value *x5 = (Value *) 5;
const Value *x6 = (Value *) 6;
const Value *x7 = (Value *) 7;
const Value *x8 = (Value *) 8;
const Value *x9 = (Value *) 9;
const Value *x10 = (Value *) 10;
const Value *x11 = (Value *) 11;
const Value *x12 = (Value *) 12;

const std::map<Value const *, NormalizedConjunction::Equality> E1 = {
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

const std::map<Value const *, NormalizedConjunction::Equality> E2 = {
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

auto mapToSeccond = [](std::pair<Value const*, NormalizedConjunction::Equality> p){ return p.second; };


bool NormalizedConjunctionTest::runTestAll() {
    std::cout << "Testing all: ";
    bool result = false;
    std::map<Value const*, NormalizedConjunction::Equality> expected = {
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x3, 15}},
        {x7, {x7, 1, x6, -1}},
        {x10, {x10, 2, x9, 2}},
        {x12, {x12, 2, x11, 1}}
    };
    
    NormalizedConjunction leastUpperBound = NormalizedConjunction::leastUpperBound(NormalizedConjunction(E1), NormalizedConjunction(E2));
    
    result = leastUpperBound.equalaties == expected;
    
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX0() {
    std::cout << "Testing X0: ";
    bool result = true;
    
    std::set<NormalizedConjunction::Equality> expected = {
        {x4, 3, x2, 5}
    };
    
    std::set<NormalizedConjunction::Equality> E1Set, E2Set;
    transform(E1, std::inserter(E1Set, E1Set.end()), mapToSeccond);
    transform(E2, std::inserter(E2Set, E2Set.end()), mapToSeccond);
    
    auto actual = NormalizedConjunction::computeX0(E1Set, E2Set);
    
    result = actual == expected;
    std::cout << (result ? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX1() {
    std::cout << "Testing X1: ";
    bool result = false;
    
    std::set<NormalizedConjunction::Equality> expected = {
        {x10, 2, x9, 2}
    };
    
    std::set<NormalizedConjunction::Equality> E1Set, E2Set;
    transform(E1, std::inserter(E1Set, E1Set.end()), mapToSeccond);
    transform(E2, std::inserter(E2Set, E2Set.end()), mapToSeccond);
    
    auto actual = NormalizedConjunction::computeX1(E1Set, E2Set);
    
    result = actual == expected;
    std::cout << (result ? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX2() {
    std::cout << "Testing X2: ";
    bool result = false;
    
    std::map<Value const*, NormalizedConjunction::Equality> expected = {
        
    };
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX4() {
    std::cout << "Testing X4: ";
    bool result = false;
    
    std::map<Value const*, NormalizedConjunction::Equality> expected = {
        
    };
    
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

int main() {
        
    return !(NormalizedConjunctionTest::runTestX0()
             && NormalizedConjunctionTest::runTestX1()
//             && NormalizedConjunctionTest::runTestX2()
//             && NormalizedConjunctionTest::runTestX4()
//             && NormalizedConjunctionTest::runTestAll()
             );
}
