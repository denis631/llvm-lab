#pragma once

#include "global.h"

#include <vector>

using namespace std;

namespace pcpo {

/// Matrix. Row and column are indexed beginning at 0
template<typename T> class Matrix {
public:
    //MARK: - Initializers
    /// Creates a matrix with dimensions  height x width and initalizes its values to `value`
    /// @param width Width of the matrix
    /// @param height Height of the matrix
    /// @param value Initial value for each element
    Matrix<T>(int height, int width, T value) {
        this->width = width;
        this->height = height;
        this->vectors.reserve(width);
        for (int i = 0; i < height; i++) {
            vector<T> vector(width,value);
            vectors.push_back(vector);
        }
    };

    /// Creates a matrix with dimensions  height x width and initalizes its values to 0
    /// @param width Width of the matrix
    /// @param height Height of the matrix
    Matrix<T>(int height, int width) : Matrix<T>(height, width, 0) {};

    Matrix<T>() = default;
    Matrix(Matrix const& matrix) = default;

    /// Creates an identity matrix with dimension eye x eye
    /// @param eye dimension of the matrix
    Matrix(int eye){
        this->width = eye;
        this->height = eye;
        this->vectors.reserve(width);
        for (int i = 0; i < height; i++) {
            vector<T> vector(width,0);
            vector[i] = 1;
            vectors[i] = vector;
        }
    };

    /// Creates a matrix from a 2D vector
    /// @param vectors 2D vector containing columns with rows
    Matrix(vector<vector<T>> const &vectors) {
        this->height = vectors.size();
        this->width = vectors.front().size();
        this->vectors = vectors;
    };

    /// Creates a vector from a std::vector
    /// @param vector the vector
    Matrix(vector<T> const &vector) {
        std::vector<std::vector<T>> vectors = {vector};
        this->vectors = vectors;
    };

    // MARK: - Properties

    /// The height of the matrix (number of rows)
    int getHeight() const { return height; };
    /// The width of the matrix (number of columns)
    int getWidth() const { return width; };
    /// Returns true when the matrix is empty
    bool empty() const { return getWidth() == 0 && getHeight() == 0; };

    // MARK: - Matrix operations

    /// Transposes the matrix
    Matrix transpose() const {
        Matrix result = Matrix(width, height);
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                result.vectors[j][i] = vectors[i][j];
            }
        }
        return result;
    };

    /// Transforms the matrix to reduced row echelon form
    Matrix echelon() const {
        Matrix result = Matrix(*this);
        int pivot = 0;
        for (int row = 0; row < height; row++) {
            if (pivot >= width) { return result; }
            int i = row;
            while (result.vectors[i][pivot] == 0) {
                if (++i >= height) {
                    i = row;
                    if (++pivot >= width) { return result; }
                }
            }
            result.swap_rows(i, row);
            result.divide_row(row, result.vectors[row][pivot]);
            for (i = 0; i < height; i++) {
                if (i != row) {
                    result.add_multiple_row(i, row, -result.vectors[i][pivot]);
                }
            }
        }
        return result;
    };

    /// The rank of the matrix
    int getRank() const {
        Matrix e = echelon();
        int rank = 0;
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                if ((e.vectors[row][column] == 0) && (column == width - 1)) {
                    return rank;
                } else if (e.vectors[row][column] != 0) {
                    break;
                }
            }
            rank++;
        }
        return rank;
    }

    /// Linear span of the matrix ... fixme
    Matrix span() const {
        vector<vector<T>> columns;
        int rank = getRank();
        for (int col = 0; col<rank; col++) {
            columns.push_back(column(col));
        }
        return Matrix(columns).transpose();
    }

    /// Converts the matrix to a 1D Vector by stacking the column vectors
    Matrix<T> toVector() const {
        vector<T> result;
        result.reserve(getWidth() * getHeight());
        for (vector<T> vector: vectors) {
            result.insert(result.end(), vector.begin(), vector.end());
        }
        return Matrix(result);
    }

    /// Converts a 1D Vector to a Matrix with given dimensions
    /// @param rows number of rows
    /// @param columns number of columns
    Matrix<T> reshape(int rows, int columns) const {
        assert(getWidth() == 1 && getHeight() == rows * columns);
        vector<vector<T>> result;
        result.reserve(rows);
        for (int row = 0; row < rows; row++) {
            vector<T> rowVector;
            rowVector.reserve(columns);
            for (int column = 0; column < columns; column++) {
                rowVector.push_back(value(row,column));
            }
        }
        return Matrix(result);
    }
    /// Returns a vector with the elements of the row at index i. The returned row can be modified.
    /// @param i Index of the row to return.
    vector<T>& row(int i) { return vectors[i]; };

    /// Returns the column at index i. The returned column cannot be modified.
    /// @param i Index of the column to return
    vector<T> column(int i) const {
        vector<T> row;
        row.reserve(width);
        for (vector<T> const& x : vectors) {
            row.push_back(x[i]);
        }
        return row;
    }

    // MARK: - Operators

    T& operator()(int i, int j) { return vectors[i][j]; };

    Matrix<T>& operator *=(Matrix<T> const& rhs) {
        assert(width == rhs.height);
        Matrix result = Matrix(height,rhs.width);
        for (int i = 0; i < height; i++) {
            for (int k = 0; k < width; k++) {
                for (int j = 0; j < rhs.width; j++) {
                    result.vectors[i][j] += vectors[i][k] * rhs.vectors[k][j];
                }
            }
        }
        this->vectors = result.vectors;
        this->width = result.width;
        this->height = result.height;
        return *this;
    };

    Matrix<T>& operator *=(T scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                vectors[i][j] *= scalar;
            }
        }
        return *this;
    };


    Matrix<T>& operator +=(Matrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i=0;i<height;i++) {
            for (int j = 0; j < width; j++) {
                vectors[i][j] += rhs.vectors[i][j];
            }
        }
        return *this;
    };

    Matrix<T>& operator +=(T scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                vectors[i][j] += scalar;
            }
        }
        return *this;
    };

    Matrix<T>& operator -=(Matrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i = 0; i < height; i++) {
            for (int j= 0; j < width; j++) {
                vectors[i][j] -= rhs.vectors[i][j];
            }
        }
        return *this;
    };

    Matrix<T>& operator -=(int scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                vectors[i][j] -= scalar;
            }
        }
        return *this;
    };

    bool operator==(Matrix<T> const& rhs) const {
        return rhs.vectors == rhs.vectors && width == rhs.width && height == rhs.height;
    };

protected:
    vector<vector<T>> vectors;
    int width;
    int height;

    // MARK: - Echelon helpers

    /// Swaps two rows
    /// @param a index of the first row
    /// @param b index of the second row
    void swap_rows(int a, int b) { vectors[a].swap(vectors[b]); };

    /// Divides a row by a constant
    /// @param row  index of the row to divide
    /// @param quotient quotient to divide the row by
    void divide_row(int row, int quotient) {
        for (int column = 0; column < width; column++) {
            vectors[row][column] /= quotient;
        }
    };

    /// Adds a multiple of row b to a row a
    /// @param a Row to add a multiple of b to
    /// @param b Row to be added to row a
    /// @param factor Factor to multiply row b with when adding it to row a
    void add_multiple_row(int a, int b, int factor) {
        for (int column = 0; column < width; column++) {
            vectors[a][column] += vectors[b][column] * factor;
        }
    };

    // MARK: - Utils

    /// Greates common divisor.
    /// @param lhs
    /// @param rhs
    static int ggT(int lhs, int rhs) {
        int h;
        if (lhs == 0) { return abs(rhs); }
        if (rhs == 0) { return abs(lhs); }

        do {
            h = lhs % rhs;
            lhs = rhs;
            rhs = h;
        } while (rhs != 0);

        return abs(lhs);
    };

    /// Least common multiple.
    /// @param lhs
    /// @param rhs
    static int kgV(int lhs, int rhs) { return (lhs * rhs) / ggT(lhs, rhs); };

    
};


template <typename T>
inline Matrix<T> operator*(Matrix<T> lhs, Matrix<T> const& rhs) { return lhs *= rhs; };
template <typename T>
inline Matrix<T> operator*(Matrix<T> lhs, T scalar) { return lhs *= scalar; };
template <typename T>
inline Matrix<T> operator+(Matrix<T> lhs, Matrix<T> const& rhs) { return lhs += rhs; };
template <typename T>
inline Matrix<T> operator+(Matrix<T> lhs, T scalar) { return lhs += scalar; };
template <typename T>
inline Matrix<T> operator-(Matrix<T> lhs, Matrix<T> const& rhs) { return  lhs -= rhs; };
template <typename T>
inline Matrix<T> operator-(Matrix<T> lhs, T scalar) { return lhs -= scalar; };

}

