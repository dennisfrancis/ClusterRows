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
#include "diagonal.hxx"

#include <stdexcept>
#include <cmath>

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
                val += at(row, inner) * right.at(inner, col);
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
Matrix Matrix::dot(const DiagonalMatrix& right) const
{
    if (m_cols != right.m_size)
    {
        throw std::runtime_error("dot: A.cols != BDiag.size");
    }

    Matrix res(m_rows, m_cols);
    int index = 0;
    for (int row = 0; row < m_rows; ++row)
    {
        for (int col = 0; col < m_cols; ++col)
        {
            res.m_data[index] = m_data[index] * right.m_data[col];
            ++index;
        }
    }
    return res;
}
Matrix Matrix::givens_rot(int col1, int col2, double theta) const
{
    if (col1 < 0 || col1 >= m_cols || col2 < 0 || col2 >= m_cols || col1 == col2)
    {
        throw std::runtime_error("givens_rot: invalid col1 and/or col2");
    }
    if (col1 > col2)
        std::swap(col1, col2);

    Matrix res(m_rows, m_cols);

    double c = std::cos(theta);
    double s = std::sin(theta);

    int res_index = 0;
    int row_begin_index = 0;
    for (int i = 0; i < m_rows; ++i)
    {
        for (int j = 0; j < m_cols; ++j)
        {
            double& res_val = res.m_data[res_index];
            if (j != col1 && j != col2)
                res_val = m_data[res_index];
            else if (j == col1)
                res_val = m_data[res_index] * c + m_data[row_begin_index + col2] * s;
            else /* j == col2 */
                res_val = m_data[res_index] * c - m_data[row_begin_index + col1] * s;

            ++res_index;
        }
        row_begin_index += m_cols;
    }
    return res;
}

}
