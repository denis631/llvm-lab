#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../src/linear_subspace.h"

using namespace std;
using namespace pcpo;

class LinearSubspaceTest: LinearSubspace {

public:
    static bool runTestLeastUpperBound1();
    static bool runTestLeastUpperBound2();
};

const llvm::Value *x1 = (llvm::Value *) 1;
const llvm::Value *x2 = (llvm::Value *) 2;
const llvm::Value *x3 = (llvm::Value *) 3;

const std::unordered_map<llvm::Value const *, int> mock_index = {
    {x1, 1},
    {x2, 2},
    {x3, 3},
};

bool LinearSubspaceTest::runTestLeastUpperBound1() {
    std::cout << "Testing least upper bound 1: ";
    bool result = false;

    LinearSubspace r1 = LinearSubspace();
    r1.isBottom = false;
    r1.basis = {MatrixType(4)};
    r1.index = mock_index;

    LinearSubspace r2 = LinearSubspace();
    r2.isBottom = false;
    r2.basis = {MatrixType(4)};
    r2.index = mock_index;

    LinearSubspace expected = LinearSubspace();
    expected.basis = {MatrixType(4)};


    auto actual = r1.leastUpperBound(r2);

    result = r1.basis == expected.basis && !actual && !r1.isBottom;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

bool LinearSubspaceTest::runTestLeastUpperBound2() {
    std::cout << "Testing least upper bound 2: ";
    bool result = false;

    LinearSubspace r1 = LinearSubspace();
    r1.isBottom = false;
    MatrixType b1 = MatrixType(4);
    b1.setValue(0,1, 1);
    b1.setValue(2,1, 1);
    r1.basis = {b1};
    r1.index = mock_index;

    LinearSubspace r2 = LinearSubspace();
    r2.isBottom = false;
    MatrixType b2 = MatrixType(4);
    b2.setValue(0,3, 1);
    r2.basis = {b2};
    r2.index = mock_index;

    LinearSubspace expected = LinearSubspace();
    MatrixType e1 =  MatrixType(4);
    e1.setValue(0,3, 1);
    MatrixType e2 = MatrixType(4);
    e2.setValue(0,0, 0);
    e2.setValue(1,1, 0);
    e2.setValue(2,2, 0);
    e2.setValue(3,3, 0);
    e2.setValue(0,1, 1);
    e2.setValue(2,1, 1);
    e2.setValue(0,3, -1);
    expected.basis = {e1, e2};


    auto actual = r1.leastUpperBound(r2);

    result = r1.basis == expected.basis && actual && !r1.isBottom;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}


int main() {
    return !(LinearSubspaceTest::runTestLeastUpperBound1()
             && LinearSubspaceTest::runTestLeastUpperBound2()
    );
};


