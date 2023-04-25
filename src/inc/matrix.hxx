/*
 * ClusterRows
 * Copyright (c) 2023 Dennis Francis <dennisfrancis.in@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <memory>
#include <cassert>

namespace util
{
class DiagonalMatrix;

class Matrix
{
public:
    Matrix(const int rows, const int cols)
        : m_data(std::make_unique<double[]>(rows * cols))
        , m_rows(rows)
        , m_cols(cols)
    {
    }

    Matrix(int rows, int cols, const double* matrix);

    Matrix(const Matrix& other);
    Matrix(Matrix&& other) = default;

    Matrix& operator=(const Matrix& other) = delete;
    Matrix& operator=(Matrix&& other) = default;

    [[nodiscard]] const double& at(int row, int col) const
    {
        check_bounds(row, col);
        return m_data[index(row, col)];
    }
    double& at(int row, int col)
    {
        check_bounds(row, col);
        return m_data[index(row, col)];
    }

    [[nodiscard]] Matrix dot(const Matrix& right) const;
    [[nodiscard]] Matrix dot(const DiagonalMatrix& right) const;
    [[nodiscard]] Matrix dot_inverse(const DiagonalMatrix& right) const;
    [[nodiscard]] Matrix dot_transpose(const Matrix& right) const;

    [[nodiscard]] Matrix givens_rot(int col1, int col2, double theta) const;

    [[nodiscard]] int rows() const { return m_rows; }
    [[nodiscard]] int cols() const { return m_cols; }

    void set_identity();

    [[nodiscard]] double sum_of_squares() const;
    [[nodiscard]] double cols_inner_product(int col1, int col2) const;

    [[nodiscard]] Matrix inverse() const;

    void display() const;

    friend bool operator==(const Matrix& m1, const Matrix& m2);

private:
    std::unique_ptr<double[]> m_data;
    int m_rows;
    int m_cols;

    void check_bounds(int row, int col) const;
    [[nodiscard]] inline size_t index(int row, int col) const { return row * m_cols + col; }
    [[nodiscard]] Matrix dot_impl(const DiagonalMatrix& right, bool inverse = false) const;
};
}
