#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../src/matrix.h"

using namespace std;
using namespace pcpo;

template <typename T>
class MatrixTest: Matrix<T> {

public:
    static bool runTestMul1();
    static bool runTestMul2();
    static bool runTestTranspose1();
    static bool runTestTranspose2();
    static bool runTestEchelon1();
    static bool runTestEchelon2();
    static bool runTestEchelon3();
};

template <typename T>
bool MatrixTest<T>::runTestMul1() {
    std::cout << "Testing multiplication 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,4,7},
        {2,5,8},
        {3,6,9}
    };

    std::vector<std::vector<T>> b = {
        {4,29,0},
        {-1,27,2},
        {100,5,3}
    };

    std::vector<std::vector<T>> expected = {
        {700,172,29},
        {803,233,34},
        {906,294,39}
    };

    auto actual = Matrix(a) * Matrix(b);

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template <typename T>
bool MatrixTest<T>::runTestMul2() {
    std::cout << "Testing multiplication 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,6,11},
        {2,7,12},
        {3,8,13},
        {4,9,14},
        {5,10,-9}
    };

    std::vector<std::vector<T>> b = {
        {43,45,1,9},
        {224,7,-2,24},
        {12,1,13,-6}
    };

    std::vector<std::vector<T>> expected = {
        {1519,87,132,87},
        {1798,139,144,114},
        {2077,191,156,141},
        {2356,243,168,168},
        {2347,295,-132,339}
    };

    auto actual = Matrix(a) * Matrix(b);

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestTranspose1() {
    std::cout << "Testing transpose 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,3},
        {4,5,6},
        {7,8,9}
    };

    std::vector<std::vector<T>> expected = {
        {1,4,7},
        {2,5,8},
        {3,6,9}
    };

    auto matrix = Matrix(a);
    auto actual = matrix.transpose();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestTranspose2() {
    std::cout << "Testing transpose 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,3},
        {2,4},
        {3,5}
    };

    std::vector<std::vector<T>> expected = {
        {1,2,3},
        {4,5,6}
    };

    auto matrix = Matrix(a);
    auto actual = matrix.transpose();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestEchelon1() {
    std::cout << "Testing echelon 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,4,7},
        {2,5,8},
        {3,6,9}
    };

    std::vector<std::vector<T>> expected = {
        {1,0,-1},
        {0,1,2},
        {0,0,0}
    };

    auto matrix = Matrix(a);
    auto actual = matrix.echelon();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestEchelon2() {
    std::cout << "Testing echelon 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,1},
        {1,4,8},
        {1,6,3}
    };

    std::vector<std::vector<T>> expected = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };

    auto matrix = Matrix(a);
    auto actual = matrix.echelon();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestEchelon3() {
    std::cout << "Testing echelon 3: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,4},
        {2,4,8},
        {4,8,16}
    };

    std::vector<std::vector<T>> expected = {
        {1,2,4},
        {0,0,0},
        {0,0,0}
    };

    auto matrix = Matrix(a);
    auto actual = matrix.echelon();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}


int main() {
    return !(MatrixTest<int>::runTestMul1()
             && MatrixTest<int>::runTestMul2()
             && MatrixTest<int>::runTestTranspose1()
             && MatrixTest<int>::runTestTranspose2()
             && MatrixTest<int>::runTestEchelon1()
             && MatrixTest<int>::runTestEchelon2()
             && MatrixTest<int>::runTestEchelon3()
    );
};

