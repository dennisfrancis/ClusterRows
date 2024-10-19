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
#include <matrix.hxx>
#include <diagonal.hxx>
#include <svd.hxx>
#include <cmath>

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

TEST(UtilTests, MatrixMultiplicationException)
{
    constexpr int size1 = 2;
    constexpr int size2 = 3;
    constexpr double matA[size1][size2] = { { 1, 3, 1 }, { 1, 7, 3 } };
    constexpr double matB[size1][size2] = { { 1, 1, 1 }, { 1, 2, 3 } };
    util::Matrix mA(size1, size2, reinterpret_cast<const double*>(matA));
    util::Matrix mB(size1, size2, reinterpret_cast<const double*>(matB));
    EXPECT_ANY_THROW(auto x = mA.dot(mB));
}

TEST(UtilTests, MatrixMultiplication)
{
    constexpr int size1 = 2;
    constexpr int size2 = 3;
    constexpr double matA[size1][size1] = { { 1, 2 }, { 3, 4 } };
    constexpr double matB[size1][size2] = { { 1, 1, 1 }, { 1, 2, 3 } };
    constexpr double exp_res[size1][size2] = { { 3, 5, 7 }, { 7, 11, 15 } };
    util::Matrix mA(size1, size1, reinterpret_cast<const double*>(matA));
    util::Matrix mB(size1, size2, reinterpret_cast<const double*>(matB));
    util::Matrix exp_mres(size1, size2, reinterpret_cast<const double*>(exp_res));
    auto mres = mA.dot(mB);
    EXPECT_EQ(mres, exp_mres);
}

TEST(UtilTests, MatrixMultDiagonal)
{
    constexpr int rows = 3;
    constexpr int cols = 4;
    constexpr double matA[rows][cols] = { { 1, 2, 3, 4 }, { 2, 1, 3, 4 }, { 4, 3, 2, 1 } };
    constexpr double diagB[cols] = { 1, 2, 3, 4 };
    constexpr double exp_res[rows][cols] = { { 1, 4, 9, 16 }, { 2, 2, 9, 16 }, { 4, 6, 6, 4 } };
    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::DiagonalMatrix mDiag(cols, diagB);
    util::Matrix exp_mres(rows, cols, reinterpret_cast<const double*>(exp_res));

    auto mres = mA.dot(mDiag);
    EXPECT_EQ(mres, exp_mres);
}

TEST(UtilTests, MatrixMultInverseDiagonal)
{
    constexpr int rows = 3;
    constexpr int cols = 4;
    constexpr double matA[rows][cols] = { { 1, 2, 3, 4 }, { 2, 1, 3, 4 }, { 4, 3, 2, 1 } };
    constexpr double diagB[cols] = { 1, 0.5, 1.0 / 3.0, 0.25 };
    constexpr double exp_res[rows][cols] = { { 1, 4, 9, 16 }, { 2, 2, 9, 16 }, { 4, 6, 6, 4 } };
    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::DiagonalMatrix mDiag(cols, diagB);
    util::Matrix exp_mres(rows, cols, reinterpret_cast<const double*>(exp_res));

    auto mres = mA.dot_inverse(mDiag);
    EXPECT_EQ(mres, exp_mres);
}

TEST(UtilTests, MatrixGivensRotation)
{
    constexpr int rows = 3;
    constexpr int cols = 6;
    constexpr double matA[rows][cols]
        = { { 1, 2, 3, 4, 2, 5 }, { 2, 1, 3, 4, 1, 8 }, { 4, 3, 2, 1, 5, 2 } };
    double matGivens[cols][cols];
    constexpr double theta = M_PI * 35.0 / 180;
    const double c = std::cos(theta);
    const double s = std::sin(theta);

    constexpr int col1 = 1;
    constexpr int col2 = 2;
    for (int i = 0; i < cols; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            if (i == j)
            {
                if (j != col1 && j != col2)
                    matGivens[i][j] = 1.0;
                else
                    matGivens[i][j] = c;
            }
            else
            {
                if (i == col2 && j == col1)
                    matGivens[i][j] = s;
                else if (i == col1 && j == col2)
                    matGivens[i][j] = -s;
                else
                    matGivens[i][j] = 0.0;
            }
        }
    }

    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::Matrix mGivens(cols, cols, reinterpret_cast<const double*>(matGivens));
    util::Matrix expected = mA.dot(mGivens);
    util::Matrix actual = mA.givens_rot(col1, col2, theta);

    EXPECT_EQ(actual, expected);
}

TEST(UtilTests, IdentityMatrix)
{
    constexpr int size = 4;
    constexpr double matA[size][size]
        = { { 1, 2, 3, 4 }, { 2, 1, 3, 4 }, { 4, 3, 2, 1 }, { 4, 2, 3, 1 } };

    util::Matrix mA(size, size, reinterpret_cast<const double*>(matA));
    util::Matrix I(size, size);
    I.set_identity();

    EXPECT_EQ(mA.dot(I), mA);
}

TEST(UtilTests, SumOfSquares)
{
    constexpr int rows = 2;
    constexpr int cols = 3;
    constexpr double matA[rows][cols] = { { 1, 2, 3 }, { 4, 7, 5 } };

    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));

    EXPECT_EQ(mA.sum_of_squares(), 1 + 4 + 9 + 16 + 49 + 25);
}

TEST(UtilTests, ColumnsInnerProduct)
{
    constexpr int rows = 2;
    constexpr int cols = 3;
    constexpr double matA[rows][cols] = { { 1, 2, -3 }, { 4, 7, 5 } };

    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));

    EXPECT_EQ(mA.cols_inner_product(0, 2), 1 * (-3) + 4 * 5);
}

TEST(UtilTests, MatrixDotTranspose)
{
    constexpr int rows = 3;
    constexpr int cols = 4;
    constexpr double matA[rows][cols] = { { 1, 2, 3, 4 }, { 2, 1, 3, 4 }, { 4, 3, 2, 1 } };
    constexpr double matB[rows][cols] = { { 2, 3, 5, 2 }, { 3, 2, 4, 5 }, { 6, 3, 2, 1 } };
    constexpr double matBTranspose[cols][rows]
        = { { 2, 3, 6 }, { 3, 2, 3 }, { 5, 4, 2 }, { 2, 5, 1 } };
    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::Matrix mB(rows, cols, reinterpret_cast<const double*>(matB));
    util::Matrix mBT(cols, rows, reinterpret_cast<const double*>(matBTranspose));
    EXPECT_EQ(mA.dot(mBT), mA.dot_transpose(mB));
}

TEST(UtilTests, MatrixSVD)
{
    constexpr int rows = 3;
    constexpr int cols = 4;
    constexpr double matA[rows][cols] = { { 1, 2, 3, 4 }, { 2, 1, 3, 4 }, { 4, 3, 2, 1 } };
    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));

    util::SVD factors(mA);
    EXPECT_EQ(factors.U.dot(factors.S).dot_transpose(factors.V), mA);
}

TEST(UtilTests, MatrixSVD2)
{
    constexpr int rows = 3;
    constexpr int cols = 3;
    constexpr double matA[rows][cols] = { { 1, 2, 3 }, { 2, 1, 4 }, { 4, 2, 1 } };
    constexpr double matI[rows][cols] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::Matrix mI(rows, cols, reinterpret_cast<const double*>(matI));

    util::SVD factors(mA);
    EXPECT_EQ(factors.U.dot(factors.S).dot_transpose(factors.V), mA);
    //factors.display();
    EXPECT_EQ(factors.U.dot_transpose(factors.U), mI);
    EXPECT_EQ(mI.dot_transpose(factors.V).dot(factors.V), mI);
    EXPECT_EQ(factors.U.dot(factors.S)
                  .dot_transpose(factors.V)
                  .dot(factors.V)
                  .dot_inverse(factors.S)
                  .dot_transpose(factors.U),
              mI);
}

TEST(UtilTests, DiagonalSingular)
{
    constexpr int size = 3;
    constexpr double diag_singular[size] = { 1, 0.5, 0.0 };
    constexpr double diag_non_singular[size] = { 1, 0.5, 0.75 };
    util::DiagonalMatrix singular(size, diag_singular);
    util::DiagonalMatrix non_singular(size, diag_non_singular);

    EXPECT_TRUE(singular.is_singular());
    EXPECT_FALSE(non_singular.is_singular());
}

TEST(UtilTests, MatrixInverse)
{
    constexpr int rows = 3;
    constexpr int cols = 3;
    constexpr double matA[rows][cols] = { { 1, 2, 3 }, { 2, 1, 4 }, { 4, 2, 1 } };

    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    auto mAInverse = mA.inverse();

    util::Matrix mI(rows, cols);
    mI.set_identity();

    EXPECT_EQ(mA.dot(mAInverse), mI);
    EXPECT_EQ(mAInverse.dot(mA), mI);
}

TEST(UtilTests, DiagonalDeterminant)
{
    constexpr int size = 4;
    constexpr double diag[size] = { 1, 0.5, 2.0, 0.25 };

    util::DiagonalMatrix mDiag(size, diag);

    EXPECT_EQ(mDiag.determinant(), 0.25);
}

TEST(UtilTests, MatrixDeterminant)
{
    constexpr int rows = 3;
    constexpr int cols = 3;
    constexpr double matA[rows][cols] = { { 0, 2, 0 }, { 0, 0, 3 }, { 5, 0, 0 } };

    util::Matrix mA(rows, cols, reinterpret_cast<const double*>(matA));
    util::SVD factors(mA);

    EXPECT_EQ(factors.determinant(), 30.0);
}
