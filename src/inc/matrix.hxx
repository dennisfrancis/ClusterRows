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

class Matrix
{
public:
    Matrix(const int rows, const int cols)
        : m_data(std::make_unique<double[]>(rows * cols))
        , m_rows(rows)
        , m_cols(cols)
    {
    }

    Matrix(const Matrix& other) = delete;
    Matrix(Matrix&& other) noexcept
        : m_data(std::move(other.m_data))
        , m_rows(other.m_rows)
        , m_cols(other.m_cols)
    {
    }

    const double& at(int row, int col) const
    {
        assert_bounds(row, col);
        return m_data[row * m_cols + col];
    }
    double& at(int row, int col)
    {
        assert_bounds(row, col);
        return m_data[row * m_cols + col];
    }

    [[nodiscard]]
    Matrix dot(const Matrix& right) const;

private:
    std::unique_ptr<double[]> m_data;
    const int m_rows;
    const int m_cols;

#ifndef NDEBUG
    void assert_bounds(int row, int col) const
    {
        assert(row >= 0);
        assert(col >= 0);
        assert(row < m_rows);
        assert(col < m_cols);
    }
#else
    void assert_bounds(int /*row*/, int /*col*/) const {}
#endif
};
}
