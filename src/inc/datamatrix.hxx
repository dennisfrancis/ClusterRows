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

namespace util
{
class DataMatrix
{
public:
    /// @brief Accepts a externally owned double array to use it as a column major matrix.
    /// @param pRows externally owned array
    /// @param rows number of rows
    /// @param cols number of columns
    DataMatrix(const double* pRows, const int rows, const int cols)
        : pData(pRows)
        , m_rows(rows)
        , m_cols(cols)
    {
    }

    const double* operator[](int row) const { return pData + (row * m_cols); }
    [[nodiscard]] int rows() const { return m_rows; }
    [[nodiscard]] int cols() const { return m_cols; }

private:
    const double* pData;
    const int m_rows;
    const int m_cols;
};

}
