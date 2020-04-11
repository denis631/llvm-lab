#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../src/simple_matrix.h"

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
        {1519,98,132,87},
        {1798,151,144,114},
        {2077,204,156,141},
        {2356,257,168,168},
        {2347,286,-132,339}
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
        {1,4},
        {2,5},
        {3,6}
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
    auto actual = matrix.echelonForm();

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
    auto actual = matrix.echelonForm();

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
    auto actual = matrix.echelonForm();

    auto x = Matrix(expected);
    result = actual == x;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestRank1() {
    std::cout << "Testing rank 1: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,4,7},
        {2,5,8},
        {3,6,9}
    };

    int expected = 2;

    auto matrix = Matrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestRank2() {
    std::cout << "Testing rank 2: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,4},
        {2,4,8},
        {4,8,16}
    };

    int expected = 1;

    auto matrix = Matrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestRank3() {
    std::cout << "Testing rank 3: ";
    bool result = false;

    std::vector<std::vector<T>> a = {
        {1,2,1},
        {1,4,8},
        {1,6,3}
    };

    int expected = 3;

    auto matrix = Matrix(a);
    auto actual = matrix.getRank();

    result = actual == expected;

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestSpan1() {
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

    auto matrix = Matrix(a);
    auto actual = Matrix<T>::span(matrix);


    result = actual == Matrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestNull1() {
    std::cout << "Testing nullspace 1: ";
    bool result = false;


    std::vector<std::vector<T>> a = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };

    auto matrix = Matrix(a);
    auto actual = Matrix<T>::null(matrix);
    Matrix<T> expected = Matrix<T>({});

    result = actual == Matrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestNull2() {
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

    auto matrix = Matrix(a);
    auto actual = Matrix<T>::null(matrix);
    auto expected = Matrix(b);

    result = actual == Matrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestNull3() {
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

    auto matrix = Matrix(a);
    auto actual = Matrix<T>::null(matrix);
    auto expected = Matrix(b);

    result = actual == Matrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestNull4() {
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

    auto matrix = Matrix(a);
    auto actual = Matrix<T>::null(matrix);
    auto expected = Matrix(b);

    result = actual == Matrix(expected);

    std::cout << (result? "success" : "failed") << "\n";
    return result;
}

template<typename T>
bool MatrixTest<T>::runTestNull5() {
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


    auto A = Matrix(a);
    auto B = Matrix(b);
    auto actual = Matrix<T>::null(Matrix(std::vector{A,B}));
    auto expected = Matrix(ans);

    result = actual == Matrix(expected);

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
             && MatrixTest<int>::runTestRank1()
             && MatrixTest<int>::runTestRank2()
             && MatrixTest<int>::runTestRank3()
             && MatrixTest<int>::runTestSpan1()
             && MatrixTest<double>::runTestMul2()
             && MatrixTest<double>::runTestTranspose1()
             && MatrixTest<double>::runTestTranspose2()
             && MatrixTest<double>::runTestEchelon1()
             && MatrixTest<double>::runTestEchelon2()
             && MatrixTest<double>::runTestEchelon3()
             && MatrixTest<double>::runTestRank1()
             && MatrixTest<double>::runTestRank2()
             && MatrixTest<double>::runTestRank3()
             && MatrixTest<double>::runTestSpan1()
             && MatrixTest<double>::runTestNull1()
             && MatrixTest<double>::runTestNull2()
             && MatrixTest<double>::runTestNull3()
             && MatrixTest<double>::runTestNull4()
             && MatrixTest<double>::runTestNull5()
    );
};

