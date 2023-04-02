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
    constexpr int size1 = 2;
    constexpr int size2 = 3;
    constexpr double matA[size1][size1] = { { 1, 2 }, { 3, 4 } };
    constexpr double matB[size1][size2] = { { 1, 1, 1 }, { 1, 1, 1 } };
    constexpr double exp_res[size1][size2] = { { 3, 3, 3 }, { 7, 7, 7 } };
    util::Matrix mA(size1, size1, reinterpret_cast<const double*>(matA));
    util::Matrix mB(size1, size2, reinterpret_cast<const double*>(matB));
    util::Matrix exp_mres(size1, size2, reinterpret_cast<const double*>(exp_res));
    auto mres = mA.dot(mB);
    EXPECT_EQ(mres, exp_mres);
}
