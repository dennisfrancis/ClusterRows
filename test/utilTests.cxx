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

#include <gtest/gtest.h>
#include "../src/inc/matrix.hxx"

TEST(UtilTests, MatrixMoveConstructor)
{
    util::Matrix m1(5, 3);
    m1.at(1, 2) = 100;
    double& elem = m1.at(1, 2);
    util::Matrix m2(std::move(m1));
    EXPECT_EQ(m2.at(1, 2), elem);
    EXPECT_EQ(&m2.at(1, 2), &elem);
}

TEST(UtilTests, MatrixElementAccess)
{
    util::Matrix m1(5, 3);
    m1.at(4, 2) = 100.0;
    EXPECT_EQ(m1.at(4, 2), 100.0);
    m1.at(4, 2) = 300.0;
    EXPECT_EQ(m1.at(4, 2), 300.0);
    m1.at(0, 0) = -100;
    EXPECT_EQ(m1.at(4, 2), 300.0);
    EXPECT_EQ(m1.at(0, 0), -100.0);
    EXPECT_ANY_THROW(m1.at(5, 3));
}

TEST(UtilTests, MatrixMultiplication)
{
    util::Matrix m1(2, 2);
    constexpr int size = 2;
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            m1.at(i, j) = (i == j) ? (i + 1) : 0;
    auto mres = m1.dot(m1);
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            auto x = int(mres.at(i, j));
            if (i == j)
                EXPECT_EQ(x, (i + 1) * (i + 1));
            else
                EXPECT_EQ(x, 0);
        }
    }
}
