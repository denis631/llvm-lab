#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../src/sparse_matrix.h"

using namespace std;
using namespace pcpo;

template <typename T>
class SparseMatrixTest: SparseMatrix<T> {

public:
    static bool runTestMul1();
    static bool runTestMul2();
    static bool runTestTranspose1();
    static bool runTestTranspose2();
    static bool runTestEchelon1();
    static bool runTestEchelon2();
    static bool runTestEchelon3();
    static bool runTestRank1();
    static bool runTestRank2();
    static bool runTestRank3();
    static bool runTestSpan1();
    static bool runTestNull1();
    static bool runTestNull2();
    static bool runTestNull3();
    static bool runTestNull4();
    static bool runTestNull5();
};

template <typename T>
bool SparseMatrixTest<T>::runTestMul1() {
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

    auto actual = SparseMatrix(a) * SparseMatrix(b);

    auto x = SparseMatrix(expected);
    result = actual == x;


    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template <typename T>
bool SparseMatrixTest<T>::runTestMul2() {
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
        {1519,98,132,87},
        {1798,151,144,114},
        {2077,204,156,141},
        {2356,257,168,168},
        {2347,286,-132,339}
    };

    auto actual = SparseMatrix(a) * SparseMatrix(b);

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestTranspose1() {
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

    auto matrix = SparseMatrix(a);

    auto actual = matrix.transpose();

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestTranspose2() {
    std::cout << "Testing transpose 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,4},
        {2,5},
        {3,6}
    };

    std::vector<std::vector<T>> expected = {
        {1,2,3},
        {4,5,6}
    };

    auto matrix = SparseMatrix(a);
    auto actual = matrix.transpose();

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestEchelon1() {
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

    auto matrix = SparseMatrix(a);
    auto actual = matrix.echelonForm();

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestEchelon2() {
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

    auto matrix = SparseMatrix(a);
    auto actual = matrix.echelonForm();

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestEchelon3() {
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

    auto matrix = SparseMatrix(a);
    auto actual = matrix.echelonForm();

    auto x = SparseMatrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestRank1() {
    std::cout << "Testing rank 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,4,7},
        {2,5,8},
        {3,6,9}
    };

    int expected = 2;

    auto matrix = SparseMatrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestRank2() {
    std::cout << "Testing rank 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,4},
        {2,4,8},
        {4,8,16}
    };

    int expected = 1;

    auto matrix = SparseMatrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestRank3() {
    std::cout << "Testing rank 3: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,1},
        {1,4,8},
        {1,6,3}
    };

    int expected = 3;

    auto matrix = SparseMatrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestSpan1() {
    std::cout << "Testing span 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,1,4},
        {0,1,4},
        {1,0,0}
    };

    std::vector<std::vector<T>> expected = {
        {1,0},
        {0,1},
        {1,-1}
    };

    auto matrix = SparseMatrix(a);
    auto actual = SparseMatrix<T>::span(matrix);


    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestNull1() {
    std::cout << "Testing nullspace 1: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };

    auto matrix = SparseMatrix(a);
    auto actual = SparseMatrix<T>::null(matrix);
    SparseMatrix<T> expected = SparseMatrix<T>({});

    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestNull2() {
    std::cout << "Testing nullspace 2: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {1,-10, -24, -42},
        {1,-8,-18,-32},
        {-2,20,51,87}
    };

    std::vector<std::vector<T>> b = {
        {2},
        {2},
        {1},
        {-1}
    };

    auto matrix = SparseMatrix(a);
    auto actual = SparseMatrix<T>::null(matrix);
    auto expected = SparseMatrix(b);

    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestNull3() {
    std::cout << "Testing nullspace 3: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {0,1,0,0,-2,-13},
        {0,0,0,1, 2, 5},
        {0,0,1,0, 1, 9}
    };

    std::vector<std::vector<T>> b = {
        {-1, 0,   0},
        {0, -2, -13},
        {0,  1,   9},
        {0,  2,   5},
        {0, -1,   0},
        {0,  0,  -1}
    };

    auto matrix = SparseMatrix(a);
    auto actual = SparseMatrix<T>::null(matrix);
    auto expected = SparseMatrix(b);

    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestNull4() {
    std::cout << "Testing nullspace 4: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {0,0,1,0,0,0,0,-2,-13},
        {0,0,0,0,0,0,1, 2, 5},
        {0,0,0,0,0,1,0, 1, 9}
    };

    std::vector<std::vector<T>> b = {
        {-1,0,0,0, 0, 0},
        {0,-1,0,0, 0, 0},
        {0,0,0,0, -2,-13},
        {0,0,-1,0, 0, 0},
        {0,0,0,-1, 0, 0},
        {0,0,0,0,1,9},
        {0,0,0,0,2,5},
        {0,0,0,0,-1, 0},
        {0,0,0,0, 0, -1}
    };

    auto matrix = SparseMatrix(a);
    auto actual = SparseMatrix<T>::null(matrix);
    auto expected = SparseMatrix(b);

    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool SparseMatrixTest<T>::runTestNull5() {
    std::cout << "Testing nullspace 5: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {0,1,1},
        {0,0,1},
        {0,0,0}
    };

    std::vector<std::vector<T>> b = {
        {0,0,0},
        {0,0,1},
        {0,0,0}
    };

    std::vector<std::vector<T>> ans = {
        {-1},
        {0},
        {0}
    };


    auto A = SparseMatrix(a);
    auto B = SparseMatrix(b);
    auto actual = SparseMatrix<T>::null(SparseMatrix(std::vector{A,B}));
    auto expected = SparseMatrix(ans);

    result = actual == SparseMatrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}



int main() {
    return !(SparseMatrixTest<int>::runTestMul1()
             && SparseMatrixTest<int>::runTestMul2()
             && SparseMatrixTest<int>::runTestTranspose1()
             && SparseMatrixTest<int>::runTestTranspose2()
             && SparseMatrixTest<int>::runTestEchelon1()
             && SparseMatrixTest<int>::runTestEchelon2()
             && SparseMatrixTest<int>::runTestEchelon3()
             && SparseMatrixTest<int>::runTestRank1()
             && SparseMatrixTest<int>::runTestRank2()
             && SparseMatrixTest<int>::runTestRank3()
             && SparseMatrixTest<int>::runTestSpan1()
             && SparseMatrixTest<double>::runTestMul2()
             && SparseMatrixTest<double>::runTestTranspose1()
             && SparseMatrixTest<double>::runTestTranspose2()
             && SparseMatrixTest<double>::runTestEchelon1()
             && SparseMatrixTest<double>::runTestEchelon2()
             && SparseMatrixTest<double>::runTestEchelon3()
             && SparseMatrixTest<double>::runTestRank1()
             && SparseMatrixTest<double>::runTestRank2()
             && SparseMatrixTest<double>::runTestRank3()
             && SparseMatrixTest<double>::runTestSpan1()
             && SparseMatrixTest<double>::runTestNull1()
             && SparseMatrixTest<double>::runTestNull2()
             && SparseMatrixTest<double>::runTestNull3()
             && SparseMatrixTest<double>::runTestNull4()
             && SparseMatrixTest<double>::runTestNull5()
             );
};


