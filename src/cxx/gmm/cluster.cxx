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

#include <cfloat>
#include <gmm/cluster.hxx>

#include <cmath>
#include <iostream>
#include <chrono>
#include <random>

gmm::Cluster::Cluster(int idx_, const Data& data_, int num_clusters_)
    : data{ data_ }
    , mu(data_.cols(), 1)
    , sigma(data_.cols(), data_.cols())
    , num_clusters{ num_clusters_ }
    , idx{ idx_ }
{
}

void gmm::Cluster::init(int use_sample)
{
    // std::cerr << "[DEBUG] inside Cluster::init() use_sample = " << use_sample << '\n';
    const int n = dims();
    const int c = clusters();

    phi = 1.0 / c;

    mu = data(use_sample).reshaped(n, 1);
    sigma.setIdentity();
    sigma *= 5;
}

void gmm::Cluster::init_cheat()
{
    const int c = clusters();
    const int n = dims();

    phi = 1.0 / c;
    // Original truth
    const double centers[3][5]
        = { { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 3.0, 4.0, 5.0, 1.0, 2.0 }, { 5.0, 1.0, 2.0, 3.0, 4.0 } };
    // const double centers[3][5]
    //     = { { 3.0, 4.0, 5.0, 1.0, 2.0 }, { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 5.0, 1.0, 2.0, 3.0, 4.0 } };

    double stds[3][5] = {
        { 2.5, 1.5, 2.5, 1.5, 1.5 },
        { 0.2, 1.5, 1.2, 0.5, 1.2 },
        { 0.7, 2.1, 1.5, 0.7, 1.5 },
    };

    sigma.setZero();
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::normal_distribution<double> normalSampler(0, 0.05);
    for (int dim = 0; dim < n; ++dim)
    {
        mu(dim, 0) = centers[idx][dim] + normalSampler(generator);
        sigma(dim, dim) = stds[idx][dim];
    }
}

void gmm::Cluster::clear_mu_sigma()
{
    // std::cerr << "[DEBUG] inside Cluster::clear_mu_sigma()\n";
    mu.setZero();
    sigma.setZero();
}

namespace
{
static const double norm_dist_scale = 1.0 / std::sqrt(2 * M_PI);
double dnorm(double x, double mean, double stdev)
{
    const double scale = norm_dist_scale / stdev;
    double arg = (x - mean) / stdev;
    return std::exp(-0.5 * (arg * arg)) * scale;
}
}

#define USE_VECTORIZED 1

double gmm::Cluster::sample_probability(int sample, const MatrixXd& cov_inv,
                                        const double cov_determinant) const
{
    // if (sample == 0)
    //     std::cerr << "<<<Data>>> " << data(sample).reshaped(1, dims()) << '\n';
    double prob;
    const int n = dims();
    (void)(n);
    (void)(dnorm);

#ifdef USE_VECTORIZED
    MatrixXd X = data(sample).reshaped(dims(), 1);
    auto res = (X - mu).transpose() * cov_inv * (X - mu);
    double exp_arg{ -0.5 * res(0, 0) };
    double density = (exp_arg < DBL_MAX_EXP) ? std::exp(exp_arg) / std::pow(2 * M_PI, dims() / 2.0)
                                                   / std::sqrt(cov_determinant)
                                             : 0;

    prob = density * phi;
    // if (sample == 0)
    //     std::cerr << "(" << res.rows() << ',' << res.cols() << ") res(0,0) = " << res(0, 0);

    // if (sample == 0)
    //     std::cerr << '\n';
    // if (sample < 5)
    //     std::cerr << "[exp_arg = " << exp_arg << " density = " << density << ", phi = " << phi
    //               << " prob = " << prob << "]  ";
    // if (sample == 5)
    //     std::cerr << '\n';
#else
    prob = phi;
    for (int dim = 0; dim < n; ++dim)
    {
        prob *= dnorm(data(sample, dim), mu(dim, 0), std::sqrt(sigma(dim, dim)));
    }
#endif

    return prob;
}

namespace gmm
{
std::ostream& operator<<(std::ostream& os, const gmm::Cluster& clusterObj)
{
    os << "Cluster(id = " << clusterObj.idx << "): " << "data(" << clusterObj.data.rows() << ", "
       << clusterObj.data.cols() << ") \tmu(" << clusterObj.mu.rows() << ", "
       << clusterObj.mu.cols() << ") \t sigma(" << clusterObj.sigma.rows() << ", "
       << clusterObj.sigma.cols() << ") num_clusters = " << clusterObj.num_clusters
       << " dims = " << clusterObj.dims() << " samples = " << clusterObj.samples()
       << " clusters = " << clusterObj.clusters();
    return os;
}
}
