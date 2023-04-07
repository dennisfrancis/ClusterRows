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

namespace util
{

class DiagonalMatrix
{
public:
    explicit DiagonalMatrix(const int size)
        : m_data(std::make_unique<double[]>(size))
        , m_size(size)
    {
    }

    [[nodiscard]] const double& at(int index) const
    {
        check_bounds(index);
        return m_data[index];
    }
    double& at(int index)
    {
        check_bounds(index);
        return m_data[index];
    }

private:
    std::unique_ptr<double[]> m_data;
    const int m_size;

    void check_bounds(int index) const;
};

}