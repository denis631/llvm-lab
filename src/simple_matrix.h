#pragma once

#include "global.h"

#include <vector>
#include <type_traits>
#include <numeric>

namespace pcpo {

/// Matrix. Row and column are indexed beginning at 0
template<typename T> class Matrix {
protected:
    std::vector<std::vector<T>> vectors;
    int width;
    int height;

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
            std::vector<T> vector(width,value);
            vectors.push_back(vector);
        }
    };

    /// Creates a matrix with dimensions  height x width and initalizes its values to 0
    /// @param width Width of the matrix
    /// @param height Height of the matrix
    Matrix<T>(int height, int width) : Matrix<T>(height, width, 0) {};

    Matrix<T>() = default;
    Matrix(Matrix const& matrix) = default;
    virtual ~Matrix<T>() = default;


    /// Creates an identity matrix with dimension eye x eye
    /// @param eye dimension of the matrix
    Matrix(int eye){
        this->width = eye;
        this->height = eye;
        this->vectors.reserve(width);
        for (int i = 0; i < height; i++) {
            std::vector<T> vector(width,0);
            vector[i] = 1;
            vectors.push_back(vector);
        }
    };

    /// Creates a matrix from a 2D vector
    /// @param vectors 2D vector containing columns with rows
    Matrix(std::vector<std::vector<T>> const &vectors) {
        assert(all_of(vectors.begin(), vectors.end(), [&vectors](std::vector<T> vec){ return vec.size() == vectors.front().size(); }));
        this->width = vectors.empty() ? 0 : vectors.front().size();
        this->height = vectors.size();
        this->vectors = vectors;
    };

    /// Creates a column vector from a std::vector
    /// @param vector the vector
    Matrix(std::vector<T> const &vector) {
        std::vector<std::vector<T>> vectors = {vector};
        this->width = vector.size();
        this->height = vector.empty() ? 0 : 1;
        this->vectors = vectors;
    };

    Matrix(std::vector<T> const &values, int rows, int columns) {
        assert(int(values.size()) == rows * columns);
        vectors.reserve(rows);
        for (int row = 0; row < rows; row++) {
            std::vector<T> rowVector;
            rowVector.reserve(columns);
            for (int column = 0; column < columns; column++) {
                rowVector.push_back(values[row * rows + column]);
            }
            vectors.push_back(rowVector);
        }
        this->width = columns;
        this->height = rows;
    };

    Matrix(std::vector<Matrix> const &vec) {
        assert(all_of(vec.begin(), vec.end(), [&vec](Matrix m){ return m.getWidth() == vec.front().getWidth(); }));
        this->height = 0;
        int size = std::accumulate(vec.begin(), vec.end(), 0, [](int acc, Matrix m){ return acc + m.getHeight(); });
        vectors.reserve(size);
        for (auto const &m: vec) {
            vectors.insert(vectors.end(), m.vectors.begin(), m.vectors.end());
            this->height += m.height;
        }
        this->width = vec.empty() ? 0 : vec.front().width;
    }

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
                result.value(j,i) = value(i,j);
            }
        }
        return result;
    };

    /// Transposes the matrix in place
    void transposed() {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                value(j,i) = value(i,j);
            }
        }
    }

    /// Transforms the matrix to reduced row echelon form
    Matrix echelonForm() const {
        Matrix result = Matrix(*this);
        int pivot = 0;
        for (int row = 0; row < height; row++) {
            if (pivot >= width) {
                return result;
            }
            int i = row;
            while (result.value(i,pivot) == 0) {
                if (++i >= height) {
                    i = row;
                    if (++pivot >= width) {
                        return result;
                    }
                }
            }
            result.swap_rows(i, row);
            result.divide_row(row, result.value(row,pivot));
            for (i = 0; i < height; i++) {
                if (i != row) {
                    result.add_multiple_row(i, row, -result.value(i,pivot));
                }
            }
        }
        return result;
    };

    /// The rank of the matrix
    int getRank() const {
        Matrix e = echelonForm();
        int rank = 0;
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                if ((e.value(row,column) == 0) && (column == width - 1)) {
                    return rank;
                } else if (e.value(row,column) != 0) {
                    break;
                }
            }
            rank++;
        }
        return rank;
    }

    /// Basis of the linear span of the column vectors
    static Matrix<T> span(Matrix<T> const& matrix, bool transposed = false) {
        std::vector<std::vector<T>> rows;
        // if matrix is already transposed don't do it again
        Matrix<T> t = transposed ? matrix : matrix.transpose();
        Matrix<T> te = t.echelonForm();
        int rank = te.getRank();
        rows.reserve(rank);
        for (int row = 0; row < rank; row++) {
            rows.push_back(te.row(row));
        }
        return Matrix(rows).transpose();
    }

    /// Computes the null space for the column vectors
    static Matrix<T> null(Matrix<T> const& matrix) {
        auto rref = matrix.echelonForm();
        std::vector<int> nonPivotColumns;
        nonPivotColumns.reserve(matrix.getWidth());
        int offset = 0;

        for (int row = 0; row < rref.getWidth(); row++) {
            for (int column = offset; column < rref.getWidth(); column++) {
                if ((row < rref.getHeight() && rref(row,column) == 0) || row >= rref.getHeight()) {
                    std::vector<T> vector(rref.getWidth());
                    vector[column] = -1;
                    rref.vectors.insert(rref.vectors.begin()+row, vector);
                    nonPivotColumns.push_back(column);
                    rref.height += 1;
                    offset += 1;
                    row += 1;
                    if (row > rref.getHeight()) {
                        break;
                    }
                } else if (row < rref.getHeight() && rref(row,column) == 1) {
                    offset += 1;
                    break;
                }
            }
        }

        rref.height = rref.getWidth();
        rref.vectors.erase(rref.vectors.begin() + rref.getWidth(), rref.vectors.end());

        std::vector<std::vector<T>> columns;

        // pick columns for result
        for (auto column: nonPivotColumns) {
            columns.push_back(rref.column(column));
        }

        return Matrix(columns).transpose();
    }

    /// Converts the matrix to a 1D Vector by stacking the column vectors
    std::vector<T> toVector() const {
        std::vector<T> result;
        result.reserve(getWidth() * getHeight());
        for (int column = 0; column < getWidth(); column++) {
            for (int row = 0; row < getHeight(); row++) {
                result.push_back(value(row,column));
            }
        }
        return result;
    }

    /// Converts a 1D Vector to a Matrix with given dimensions
    /// @param rows number of rows
    /// @param columns number of columns
    Matrix<T> reshape(int rows, int columns) const {
        assert(rows > 0 && columns > 0);
        Matrix<T> t = transpose();
        return Matrix(t.vectors.front(), rows, columns).transpose();
    };

    std::vector<Matrix<T>> reshapeColumns(int height, int width) const {
        std::vector<Matrix<T>> result;
        for (int c = 0; c < getWidth(); c++) {
            result.push_back(Matrix(column(c), height, width).transpose());
        }
        return result;
    }

    /// Returns the value at row i and column j
    /// @param row
    /// @param column
    T& value(int row, int column) {
        assert(row < getHeight() && column < getWidth());
        return vectors[row][column];
    };

    /// Returns the value at row i and column j
    /// @param row
    /// @param column
    T const& value(int row, int column) const {
        assert(row < getHeight() && column < getWidth());
        return vectors[row][column];
    };

    /// Sets the value at row i and column j
    /// @param row
    /// @param column
    /// @param value
    void setValue(int row, int column, T val) {
        value(row,column) = val;
    }

    /// Returns a vector with the elements of the row at index i. The returned row can be modified.
    /// @param i Index of the row to return.
    std::vector<T>& row(int i) {
        assert(i < getHeight());
        return vectors[i];
    };

    std::vector<T> row(int i) const {
        assert(i < getHeight());
        return vectors[i];
    };

    /// Returns the column at index i. The returned column cannot be modified.
    /// @param i Index of the column to return
    std::vector<T> column(int i) const {
        assert(i < getWidth());
        std::vector<T> row;
        row.reserve(width);
        for (std::vector<T> const& x : vectors) {
            row.push_back(x[i]);
        }
        return row;
    }

    void setColumn(std::vector<T> const& vector, int column) {
        assert(int(vector.size()) == height);
        for (int row = 0; row < height; row++) {
            value(row,column) = vector[row];
        }
    }

    // MARK: - Operators

    T& operator()(int i, int j) { return value(i,j); };

    Matrix<T>& operator *=(Matrix<T> const& rhs) {
        assert(width == rhs.height);
        Matrix result = Matrix(height,rhs.width);
        for (int i = 0; i < height; i++) {
            for (int k = 0; k < width; k++) {
                for (int j = 0; j < rhs.width; j++) {
                    result.value(i,j) += value(i,k) * rhs.value(k,j);
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
                value(i,j) *= scalar;
            }
        }
        return *this;
    };


    Matrix<T>& operator +=(Matrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i=0;i<height;i++) {
            for (int j = 0; j < width; j++) {
                value(i,j) += rhs.value(i,j);
            }
        }
        return *this;
    };

    Matrix<T>& operator +=(T scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                value(i,j) += scalar;
            }
        }
        return *this;
    };

    Matrix<T>& operator -=(Matrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i = 0; i < height; i++) {
            for (int j= 0; j < width; j++) {
                value(i,j) -= rhs.value(i,j);
            }
        }
        return *this;
    };

    Matrix<T>& operator -=(int scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                value(i,j) -= scalar;
            }
        }
        return *this;
    };

    bool operator==(Matrix<T> const& rhs) const {
        return vectors == rhs.vectors && width == rhs.width && height == rhs.height;
    };

    virtual void print() const {
        dbgs(4) << *this;
    }

protected:

    // MARK: - Echelon helpers

    /// Swaps two rows
    /// @param a index of the first row
    /// @param b index of the second row
    void swap_rows(int a, int b) { vectors[a].swap(vectors[b]); };

    /// Divides a row by a constant
    /// @param row  index of the row to divide
    /// @param quotient quotient to divide the row by
    void divide_row(int row, T quotient) {
        for (int column = 0; column < width; column++) {
            value(row,column) /= quotient;
        }
    };

    /// Adds a multiple of row b to a row a
    /// @param a Row to add a multiple of b to
    /// @param b Row to be added to row a
    /// @param factor Factor to multiply row b with when adding it to row a
    void add_multiple_row(int a, int b, T factor) {
        for (int column = 0; column < width; column++) {
            value(a,column) += value(b,column) * factor;
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

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, Matrix<T> const& matrix) {
    for (int row = 0; row < matrix.getHeight(); row++) {
        os << "[ ";
        for (int column = 0; column < matrix.getWidth(); column++) {
            if constexpr (std::is_floating_point_v<T>) {
                if (column == matrix.getWidth() - 1) {
                    os << llvm::format("%g", matrix.value(row,column));
                } else {
                    os << llvm::format("%-6g", matrix.value(row,column));
                }
            } else {
                if (column == matrix.getWidth() - 1) {
                    os << llvm::format("%d", matrix.value(row,column));
                } else {
                    os << llvm::format("%-6d", matrix.value(row,column));
                }
            }
        }
        os << " ]\n";
    }

    if (matrix.getWidth() == 0 && matrix.getHeight() == 0) {
        os << "[]\n";
    }

    return os;
};

}

