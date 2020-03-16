#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../src/affine_relation.h"

using namespace std;
using namespace pcpo;

class AffineRelationTest: AffineRelation {

public:
    static bool runTestLeastUpperBound1();
    static bool runTestLeastUpperBound2();
};

bool AffineRelationTest::runTestLeastUpperBound1() {
    std::cout << "Testing least upper bound 1: ";
    bool result = false;

    AffineRelation r1 = AffineRelation();
    r1.isBottom = false;
    r1.basis = {Matrix<T>(4)};

    AffineRelation r2 = AffineRelation();
    r2.isBottom = false;
    r2.basis = {Matrix<T>(4)};

    AffineRelation expected = AffineRelation();
    expected.basis = {Matrix<T>(4)};


    auto actual = r1.leastUpperBound(r2);

    result = r1.basis == expected.basis && !actual && !r1.isBottom;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool AffineRelationTest::runTestLeastUpperBound2() {
    std::cout << "Testing least upper bound 2: ";
    bool result = false;

    AffineRelation r1 = AffineRelation();
    r1.isBottom = false;
    Matrix<T> b1 = Matrix<T>(4);
    b1(0,1) = 1;
    b1(2,1) = 1;
    r1.basis = {b1};

    AffineRelation r2 = AffineRelation();
    r2.isBottom = false;
    Matrix<T> b2 = Matrix<T>(4);
    b2(0,3) = 1;
    r2.basis = {b2};

    AffineRelation expected = AffineRelation();
    Matrix<T> e1 =  Matrix<T>(4);
    e1(0,3) = 1;
    Matrix<T> e2 = Matrix<T>(4);
    e2(0,0) = 0;
    e2(1,1) = 0;
    e2(2,2) = 0;
    e2(3,3) = 0;
    e2(0,1) = 1;
    e2(2,1) = 1;
    e2(0,3) = -1;
    expected.basis = {e1, e2};


    auto actual = r1.leastUpperBound(r2);

    result = r1.basis == expected.basis && actual && !r1.isBottom;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}


int main() {
    return !(AffineRelationTest::runTestLeastUpperBound1()
             && AffineRelationTest::runTestLeastUpperBound2()
    );
};


