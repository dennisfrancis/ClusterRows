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

#include <diagonal.hxx>
#include <cfloat>
#include <iostream>

namespace util
{

void DiagonalMatrix::check_bounds(int size) const
{
    if (size < 0 || size >= m_size)
        throw std::out_of_range("Out of bounds element access");
}

DiagonalMatrix::DiagonalMatrix(int size, const double* diagonal_vector)
    : DiagonalMatrix(size)
{
    for (int index = 0; index < size; ++index)
        m_data[index] = diagonal_vector[index];
}

bool DiagonalMatrix::is_singular() const
{
    static constexpr double tiny{ DBL_MIN * 100 };
    for (int index = 0; index < m_size; ++index)
        if (m_data[index] < tiny)
            return true;
    return false;
}

double DiagonalMatrix::determinant() const
{
    double prod = 1;
    for (int index = 0; index < m_size; ++index)
        prod *= m_data[index];

    return prod;
}

void DiagonalMatrix::display() const
{
    for (int i = 0; i < m_size; ++i)
        std::cout << m_data[i] << '\t';
}

}
