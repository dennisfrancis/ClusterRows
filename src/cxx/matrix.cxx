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

#include "matrix.hxx"

#include "stdexcept"

namespace util
{

Matrix Matrix::dot(const Matrix& right) const
{
    if (m_cols != right.m_rows)
    {
        throw std::runtime_error("dot: A.cols != B.rows");
    }
    Matrix res(m_rows, right.m_cols);

    for (int row = 0; row < m_rows; ++row)
    {
        for (int col = 0; col < right.m_cols; ++col)
        {
            double val = 0;
            for (int inner = 0; inner < m_cols; ++inner)
                val += at(row, inner) * at(inner, col);
            res.at(row, col) = val;
        }
    }
    return res;
}

void Matrix::check_bounds(int row, int col) const
{
    if (row < 0 || col < 0 || row >= m_rows || col >= m_cols)
        throw std::out_of_range("Out of bounds element access");
}

Matrix::Matrix(const int rows, const int cols, const double* matrix)
    : Matrix(rows, cols)
{
    for (int i = 0; i < rows; ++i)
    {
        size_t begin = i * cols;
        for (int j = 0; j < cols; ++j)
        {
            size_t idx = begin + j;
            m_data[idx] = matrix[idx];
        }
    }
}

bool operator==(const Matrix& m1, const Matrix& m2)
{
    if (m1.m_cols != m2.m_cols || m1.m_rows != m2.m_rows)
        return false;

    size_t size = m1.m_cols * m1.m_rows;
    for (size_t i = 0; i < size; ++i)
        if (std::abs(m1.m_data[i] - m2.m_data[i]) > 0.0001)
            return false;

    return true;
}

bool operator==(const Matrix& m1, const double* m2)
{
    size_t size = m1.m_cols * m1.m_rows;
    for (size_t i = 0; i < size; ++i)
        if (std::abs(m1.m_data[i] - m2[i]) > 0.0001)
            return false;
    return true;
}

}
