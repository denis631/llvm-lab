#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <dlfcn.h>

#include "../src/normalized_conjunction.h"
#include "../src/linear_equality.h"

using namespace pcpo;
using namespace llvm;


class NormalizedConjunctionTest: NormalizedConjunction {

public:
    static bool runTestAll();
    static bool runTestMerge();
    static bool runTestX0();
    static bool runTestX1();
    static bool runTestX2();
    static bool runTestX4();
    static bool runNonDeterministicAssignmentTest1();
    static bool runNonDeterministicAssignmentTest2();
    static bool runLinearAssignmentTest1();
    static bool runLinearAssignmentTest2();
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

const std::unordered_map<Value const *, LinearEquality> E1 = {
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

const std::unordered_map<Value const *, LinearEquality> E2 = {
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

auto mapToSeccond = [](std::pair<Value const*, LinearEquality> p){ return p.second; };


bool NormalizedConjunctionTest::runTestAll() {
    std::cout << "Testing all: ";
    bool result = false;
    std::unordered_map<Value const*, LinearEquality> expected = {
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x3, 15}},
        {x7, {x7, 1, x6, -1}},
        {x10, {x10, 2, x9, 2}},
        {x12, {x12, 2, x11, 1}}
    };
    
    auto actual = NormalizedConjunction(E1);
    actual.leastUpperBound(NormalizedConjunction(E2));
    
    result = actual.values == expected;
    
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestMerge() {
    std::cout << "Testing merge: ";
    bool result = false;
    std::unordered_map<Value const*, LinearEquality> expected = {
        {x4, {x4, 3, x2, 5}},
        {x5, {x5, 3, x3, 15}},
        {x7, {x7, 1, x6, -1}},
        {x10, {x10, 2, x9, 2}},
        {x12, {x12, 2, x11, 1}}
    };

    std::unordered_map<Value const *, LinearEquality> x = {
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

    std::unordered_map<Value const *, LinearEquality> y = {

    };

    auto actual = NormalizedConjunction(x);
    auto other = NormalizedConjunction(y);
    actual.merge(Merge_op::UPPER_BOUND, other);

    result = actual.values == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX0() {
    std::cout << "Testing X0: ";
    bool result = true;
    
    std::set<LinearEquality> expected = {
        {x4, 3, x2, 5}
    };
    
    std::set<LinearEquality> E1Set, E2Set;
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
    
    std::set<LinearEquality> expected = {
        {x10, 2, x9, 2}
    };
    
    std::set<LinearEquality> E1Set, E2Set;
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
    
    std::set<LinearEquality> expected = {
        {x12, 2, x11, 1}
    };
    
    std::set<LinearEquality> E1Set, E2Set;
    transform(E1, std::inserter(E1Set, E1Set.end()), mapToSeccond);
    transform(E2, std::inserter(E2Set, E2Set.end()), mapToSeccond);
    
    auto actual = NormalizedConjunctionTest::computeX2(E1Set, E2Set);
    
    result = actual == expected;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runTestX4() {
    std::cout << "Testing X4: ";
    bool result = false;
    
    std::set<LinearEquality> expected = {
        {x5, 3, x3, 15},
        {x7, 1, x6, -1}
    };
    
    std::set<LinearEquality> E1Set, E2Set;
    transform(E1, std::inserter(E1Set, E1Set.end()), mapToSeccond);
    transform(E2, std::inserter(E2Set, E2Set.end()), mapToSeccond);
    
    auto actual = NormalizedConjunctionTest::computeX4(E1Set, E2Set);
    
    result = actual == expected;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runNonDeterministicAssignmentTest1() {
    std::cout << "Testing non deterministic Assignment 1: ";
    bool result = false;
    
    NormalizedConjunction E = NormalizedConjunction({
        {x1, {x1, 1, nullptr, 4}},
        {x2, {x2, 1, nullptr, 2}}
    });
        
    auto expected = NormalizedConjunction({
        {x1, {x1, 1, nullptr, 4}},
        {x2, {x2, 1, x2, 0}},
    });
    
    E.nonDeterminsticAssignment(x2);
    
    result = E.values == expected.values;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runNonDeterministicAssignmentTest2() {
    std::cout << "Testing non deterministic Assignment 2: ";
    bool result = false;
    
    NormalizedConjunction E = NormalizedConjunction({
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x1, 2}},
        {x3, {x3, 1, x2, 4}},
        {x4, {x4, 1, x1, 10}}
    });
        
    auto expected = NormalizedConjunction({
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x2, 0}},
        {x3, {x3, 1, x2, 4}},
        {x4, {x4, 1, x2, 8}}
    });
    
    E.nonDeterminsticAssignment(x1);
    
    result = E.values == expected.values;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runLinearAssignmentTest1() {
    std::cout << "Testing linear Assignment 1: ";
    bool result = false;
    
    NormalizedConjunction E = NormalizedConjunction({
        {x1, {x1, 1, nullptr, 2}},
        {x2, {x2, 1, x2, 0}},
        {x3, {x3, 1, x2, 3}}
        
    });
        
    auto expected = NormalizedConjunction({
        {x1, {x1, 1, nullptr, 2}},
        {x2, {x2, 1, nullptr, 5}},
        {x3, {x3, 1, x3, 0}}
    });
    
    E.linearAssignment(x2, 1, x1, 3);
    
    result = E.values == expected.values;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool NormalizedConjunctionTest::runLinearAssignmentTest2() {
    std::cout << "Testing linear Assignment 2: ";
    bool result = false;
    
    NormalizedConjunction E = NormalizedConjunction({
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x1, 4}},
        {x3, {x3, 1, x3, 0}},
        {x4, {x4, 1, x3, 10}},
        {x5 ,{x5, 1, x3, -4}},
        {x6, {x6, 1, x3, 1}}
    });
        
    auto expected = NormalizedConjunction({
        {x1, {x1, 1, x1, 0}},
        {x2, {x2, 1, x2, 0}},
        {x3, {x3, 1, x2, -11}},
        {x4, {x4, 1, x2, -1}},
        {x5, {x5, 1, x2, -15}},
        {x6, {x6, 1, x2, -10}}
    });
    
    E.linearAssignment(x2, 1, x4, 1);
    
    result = E.values == expected.values;
    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

int main() {
        
    return !(NormalizedConjunctionTest::runTestX0()
             && NormalizedConjunctionTest::runTestX1()
             && NormalizedConjunctionTest::runTestX2()
             && NormalizedConjunctionTest::runTestX4()
             && NormalizedConjunctionTest::runTestAll()
             && NormalizedConjunctionTest::runTestMerge()
             && NormalizedConjunctionTest::runNonDeterministicAssignmentTest1()
             && NormalizedConjunctionTest::runNonDeterministicAssignmentTest2()
             && NormalizedConjunctionTest::runLinearAssignmentTest1()
             && NormalizedConjunctionTest::runLinearAssignmentTest2()
        );
}
