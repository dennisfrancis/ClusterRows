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

#include "svd.hxx"
#include <cmath>
#include <iostream>

namespace util
{

/**
 * SVD computes singular value decomposition of @param matrix A.
 * The factors U, S and V are computed and can be accessed directly.
 */
SVD::SVD(const Matrix& A)
    : U(A)
    , S(A.cols())
    , V(A.cols(), A.cols())
{
    V.set_identity();
    double n_sq = U.sum_of_squares();
    double s = 0;
    bool first = true;
    double eps_sq = 0.001;
    const int n = U.cols();

    while (std::sqrt(s) > (eps_sq * n_sq) || first)
    {
        s = 0;
        first = false;
        for (int i = 0; i < (n - 1); ++i)
        {
            for (int j = i + 1; j < n; ++j)
            {
                double r = U.cols_inner_product(i, j);
                s += (r * r);

                double p = U.cols_inner_product(i, i);
                double q = U.cols_inner_product(j, j);

                double theta = 0.5 * std::atan2(2 * r, p - q);

                U = U.givens_rot(i, j, theta);
                V = V.givens_rot(i, j, theta);
            }
        }
    }

    for (int i = 0; i < n; ++i)
        S.at(i) = std::sqrt(U.cols_inner_product(i, i));

    U = U.dot_inverse(S);
}

/**
 * Shows the factors U, S, V of the original matrix in the console.
 */
void SVD::display() const
{
    std::cout << "U :\n";
    U.display();

    std::cout << "\nS :\n";
    S.display();

    std::cout << "\nV :\n";
    V.display();
}

double SVD::determinant() const { return S.determinant(); }

}
