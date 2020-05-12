#pragma once

#include "global.h"
#include "general.h"

#include <vector>
#include <unordered_map>
#include <type_traits>
#include <numeric>

namespace pcpo {

/// Matrix. Row and column are indexed beginning at 0
template<typename T> class SparseMatrix {
protected:
    // (row,column) -> T
    std::unordered_map<std::pair<int,int>,T> values;
    int width;
    int height;
    bool _transposed = false;
    int getEstimatedSize() const { return width * height / 4; };

public:
    //MARK: - Initializers
    /// Creates a matrix with dimensions  height x width and initalizes its values to `value`
    /// @param width Width of the matrix
    /// @param height Height of the matrix
    /// @param value Initial value for each element
    SparseMatrix<T>(int height, int width, T value) {
        this->width = width;
        this->height = height;
        if (value == 0) return;
        values.reserve(height*value);
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                values[{row,column}] = value;
            }
        }
    }

    /// Creates a matrix with dimensions  height x width and initalizes its values to 0
    /// @param width Width of the matrix
    /// @param height Height of the matrix
    SparseMatrix<T>(int height, int width) : SparseMatrix<T>(height, width, 0) {};

    SparseMatrix<T>() = default;
    SparseMatrix(SparseMatrix const& matrix) = default;
    virtual ~SparseMatrix<T>() = default;

    /// Creates an identity matrix with dimension eye x eye
    /// @param eye dimension of the matrix
    SparseMatrix(int eye) {
        this->width = eye;
        this->height = eye;
        values.reserve(eye);
        for (int i = 0; i < eye; i++) {
            values[{i,i}] = 1;
        }
    }

    /// Creates a matrix from a 2D vector
    /// @param vectors 2D vector containing columns with rows
    SparseMatrix(std::vector<std::vector<T>> const &vectors) {
        assert(all_of(vectors.begin(), vectors.end(), [&vectors](std::vector<T> vec){ return vec.size() == vectors.front().size(); }));
        this->width = vectors.empty() ? 0 : vectors.front().size();
        this->height = vectors.size();
        values.reserve(getEstimatedSize());
        for (int row = 0; row < int(vectors.size()); row++) {
            auto rowVector = vectors[row];
            for (auto column = 0; column < int(rowVector.size()); column++) {
                T val = rowVector[column];
                if (val != 0) {
                    values[{row,column}] = val;
                }
            }
        }
    }

    SparseMatrix(std::unordered_map<std::pair<int,int>,T> const &values, int width, int height) {
        this->width = width;
        this->height = height;
        this->values = values;
    }

    /// Creates a column vector from a std::vector
    /// @param vector the vector
    SparseMatrix(std::vector<T> const &vector) {
        this->width = vector.size();
        this->height = vector.empty() ? 0 : 1;
        values.reserve(getEstimatedSize());
        for (int column = 0; column < int(vector.size()); column++) {
            T val = vector[column];
            if (val != 0) {
                values[{0,column}] = val;
            }
        }
    }

    SparseMatrix(std::vector<T> const &vs, int rows, int columns) {
        assert(int(vs.size()) == rows * columns);
        this->width = columns;
        this->height = rows;
        values.reserve(getEstimatedSize());
        for (int row = 0; row < rows; row++) {
            for (int column = 0; column < columns; column++) {
                T val = vs[row * rows + column];
                if (val != 0) {
                    values[std::pair{row,column}] = val;
                }
            }
        }
    }

    SparseMatrix(std::vector<SparseMatrix> const &vec) {
        assert(all_of(vec.begin(), vec.end(), [&vec](SparseMatrix m){ return m.getWidth() == vec.front().getWidth(); }));
        std::unordered_map<std::pair<int,int>,T> val;
        this->height = 0;
        int estimated_size = std::accumulate(vec.begin(), vec.end(), 0, [](int acc, SparseMatrix m){ return acc + m.getEstimatedSize(); });
        values.reserve(estimated_size);
        for (auto m: vec) {
            for (auto const [key, value]: m.values) {
                if (m._transposed) {
                    values[{key.second + this->height, key.first}] = value;
                } else {
                    values[{key.first + this->height, key.second}] = value;
                }
            }
            this->height += m.getHeight();
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

    /// Returns the transposed matrix
    SparseMatrix transpose() const {
        SparseMatrix<T> result = SparseMatrix(*this);
        result.transposed();
        return result;
    }

    /// Transposes the matrix
    void transposed() {
        _transposed = !_transposed;
        int oldWildth = width;
        width = height;
        height = oldWildth;
    }

    /// Transforms the matrix to reduced row echelon form
    SparseMatrix echelonForm() const {
        SparseMatrix result = SparseMatrix(*this);
        int pivot = 0;
        for (int row = 0; row < height; row++) {
            if (pivot >= width) { return result; }
            int i = row;
            while (result.value(i,pivot) == 0) {
                if (++i >= height) {
                    i = row;
                    if (++pivot >= width) { return result; }
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
    }

    /// The rank of the matrix
    int getRank() const {
        SparseMatrix e = echelonForm();
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
    static SparseMatrix<T> span(SparseMatrix<T> const& matrix, bool transposed = false) {
        std::vector<std::vector<T>> rows;
        SparseMatrix<T> t = transposed ? matrix : matrix.transpose();
        SparseMatrix<T> te = t.echelonForm();
        int rank = te.getRank();
        rows.reserve(rank);
        for (int row = 0; row < rank; row++) {
            rows.push_back(te.row(row));
        }
        return SparseMatrix(rows).transpose();
    }

    /// Computes the null space for the column vectors
    static SparseMatrix<T> null(SparseMatrix<T> const& matrix) {
        auto rref = matrix.echelonForm();

        std::unordered_map<std::pair<int,int>,T> result;
        result.reserve(matrix.getEstimatedSize());
        int index = 0;
        int offset = 0;
        std::unordered_map<int,bool> index_map;
        index_map.reserve(matrix.getWidth());

        for (int row = 0; row < rref.getWidth(); row++) {
            for (int column = offset; column < rref.getWidth(); column++) {
                if (row < rref.getHeight() && rref.value(row,column) == 1) {
                    offset += 1;
                    break;
                } else if ((row < rref.getHeight() && rref.value(row,column) == 0) || row >= rref.getHeight()) {
                    // insert -1
                    result[{column,index}] = -1;
                    index_map[column] = true;

                    // copy everyting above -1. Below is guranteed to be zero due to rref.
                    int of = 0;
                    for (int i = 0; i < std::min(row,rref.getHeight()) + of; i++) {
                        if (index_map.count(i) == 0) {
                            T value = rref.value(i-of,column);
                            if (value != 0) {
                                result[{i,index}] = value;
                            }
                        } else {
                            of += 1;
                        }
                    }

                    index += 1;
                    offset += 1;
                    if (row >= rref.getHeight()) {
                        break;
                    }
                }
            }
        }
        return SparseMatrix(result,index,result.empty() ? 0 : rref.getWidth());
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
    SparseMatrix<T> reshape(int rows, int columns) const {
        assert(rows > 0 && columns > 0);
        return SparseMatrix(column(0), rows, columns).transpose();
    }

    std::vector<SparseMatrix<T>> reshapeColumns(int height, int width) const {
        std::vector<SparseMatrix<T>> result;
        result.reserve(getWidth());
        for (int c = 0; c < getWidth(); c++) {
            result.push_back(SparseMatrix(column(c), height, width).transpose());
        }
        return result;
    }

    /// Sets the value at row i and column j
    /// @param row
    /// @param column
    /// @param value
    void setValue(int row, int column, T value) {
        auto index = _transposed ? std::pair{column,row} : std::pair{row,column};
        if (value != 0) {
            values[index] = value;
        } else {
            values.erase(index);
        }
    }

    /// Returns the value at row i and column j
    /// @param row
    /// @param column
    T value(int row, int column) const {
        assert(row < height && column < width);
        auto index = _transposed ? std::pair{column,row} : std::pair{row,column};
        auto it = values.find(index);

        if (it == values.end()) {
            return 0;
        }
        return it->second;
    }

    /// Returns a vector with the elements of the row at index i. The returned row cannot be modified.
    /// @param i Index of the row to return.
    std::vector<T> row(int i) {
        assert(i < height);
        std::vector<T> result;
        result.reserve(width);
        for (int column = 0; column < width; column++) {
            result.push_back(value(i,column));
        }
        return result;
    }

    /// Returns the column at index i. The returned column cannot be modified.
    /// @param i Index of the column to return
    std::vector<T> column(int i) const {
        assert(i < width);
        std::vector<T> result;
        result.reserve(height);
        for (int row = 0; row < height; row++) {
            result.push_back(value(row,i));
        }
        return result;
    }

    void setColumn(std::vector<T> const& vector, int column) {
        assert(int(vector.size()) == height && column < width);
        for (int row = 0; row < height; row++) {
            value(row,column) = vector[row];
        }
    }

    // MARK: - Operators

    T operator()(int i, int j) { return value(i,j); };

    SparseMatrix<T>& operator *=(SparseMatrix<T> const& rhs) {
        assert(width == rhs.height);
        SparseMatrix result = SparseMatrix(height,rhs.width);
        for (int i = 0; i < height; i++) {
            for (int k = 0; k < width; k++) {
                for (int j = 0; j < rhs.width; j++) {
                    T val = result.value(i,j) + value(i,k) * rhs.value(k,j);
                    result.setValue(i,j, val);
                }
            }
        }
        this->values = result.values;
        this->width = result.width;
        this->height = result.height;
        this->_transposed = result._transposed;
        return *this;
    }

    SparseMatrix<T>& operator *=(T scalar) {
        for (T& value : values) {
            value *= scalar;
        }
        return *this;
    }

    SparseMatrix<T>& operator +=(SparseMatrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i=0;i<height;i++) {
            for (int j = 0; j < width; j++) {
                setValue(i,j, value(i,j) + rhs.value(i,j));
            }
        }
        return *this;
    }

    SparseMatrix<T>& operator +=(T scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                setValue(i,j, value(i,j) + scalar);
            }
        }
        return *this;
    }

    SparseMatrix<T>& operator -=(SparseMatrix<T> const& rhs) {
        assert(rhs.width == width && rhs.height == height);
        for (int i = 0; i < height; i++) {
            for (int j= 0; j < width; j++) {
                setValue(i,j, value(i,j) - rhs.value(i,j));
            }
        }
        return *this;
    }

    SparseMatrix<T>& operator -=(int scalar) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                setValue(i,j, value(i,j) - scalar);
            }
        }
        return *this;
    }

    bool operator==(SparseMatrix<T> const& rhs) const {
        if (rhs._transposed != _transposed && width == rhs.width && height == rhs.height) {
            for (int row = 0; row < getHeight(); row++) {
                for (int column = 0; column < getWidth(); column++) {
                    if (value(row, column) != rhs.value(row,column)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return values == rhs.values && width == rhs.width && height == rhs.height;
    }

    virtual void print() const {
        dbgs(4) << *this;
    }

protected:

    // MARK: - Echelon helpers

    /// Swaps two rows
    /// @param a index of the first row
    /// @param b index of the second row
    void swap_rows(int a, int b) {
        for (int column = 0; column < width; column++) {
            auto nha = values.extract({a,column});
            auto nhb = values.extract({b,column});
            // check if node handle is not empty
            if (!nha.empty()) {
                nha.key() = {b,column};
                values.insert(std::move(nha));
            }
            if (!nhb.empty()) {
                nhb.key() = {a,column};
                values.insert(std::move(nhb));
            }
        }
    }

    /// Divides a row by a constant
    /// @param row  index of the row to divide
    /// @param quotient quotient to divide the row by
    void divide_row(int row, T quotient) {
        for (int column = 0; column < width; column++) {
            if (values.count({row,column}) != 0) {
                setValue(row, column, value(row,column) / quotient);
            }
        }
    }

    /// Adds a multiple of row b to a row a
    /// @param a Row to add a multiple of b to
    /// @param b Row to be added to row a
    /// @param factor Factor to multiply row b with when adding it to row a
    void add_multiple_row(int a, int b, T factor) {
        for (int column = 0; column < width; column++) {
            if (values.count({b,column}) != 0) {
                setValue(a,column, value(a,column) + value(b,column) * factor);
            }
        }
    }
};


template <typename T>
inline SparseMatrix<T> operator*(SparseMatrix<T> lhs, SparseMatrix<T> const& rhs) { return lhs *= rhs; };
template <typename T>
inline SparseMatrix<T> operator*(SparseMatrix<T> lhs, T scalar) { return lhs *= scalar; };
template <typename T>
inline SparseMatrix<T> operator+(SparseMatrix<T> lhs, SparseMatrix<T> const& rhs) { return lhs += rhs; };
template <typename T>
inline SparseMatrix<T> operator+(SparseMatrix<T> lhs, T scalar) { return lhs += scalar; };
template <typename T>
inline SparseMatrix<T> operator-(SparseMatrix<T> lhs, SparseMatrix<T> const& rhs) { return  lhs -= rhs; };
template <typename T>
inline SparseMatrix<T> operator-(SparseMatrix<T> lhs, T scalar) { return lhs -= scalar; };

template <typename T>
llvm::raw_ostream& operator<<(llvm::raw_ostream& os, SparseMatrix<T> const& matrix) {
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

