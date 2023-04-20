#include "svd.hxx"
#include <cmath>

namespace util
{

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

}
