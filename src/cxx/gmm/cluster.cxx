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
#include <optional>

gmm::Cluster::Cluster(int idx_, const Data& data_, int num_clusters_, bool full_gmm_)
    : data{ data_ }
    , mu(data_.cols(), 1)
    , num_clusters{ num_clusters_ }
    , idx{ idx_ }
    , full_gmm{ full_gmm_ }
{
    if (full_gmm)
    {
        sigma = std::make_optional<MatrixXd>(data_.cols(), data_.cols());
    }
    else
    {
        stds = std::make_optional<std::vector<double>>(data_.cols());
    }
}

void gmm::Cluster::init(int use_sample)
{
    // std::cerr << "[DEBUG] inside Cluster::init() use_sample = " << use_sample << '\n';
    const int n = dims();
    const int c = clusters();

    phi = 1.0 / c;

    mu = data(use_sample).reshaped(n, 1);
    if (full_gmm)
    {
        sigma->setIdentity();
        (*sigma) *= 5;
    }
    else
    {
        for (int dim = 0; dim < n; ++dim)
        {
            (*stds)[dim] = 1.5;
        }
    }
}

void gmm::Cluster::clear_mu_sigma()
{
    // std::cerr << "[DEBUG] inside Cluster::clear_mu_sigma()\n";
    mu.setZero();
    if (full_gmm)
    {
        sigma->setZero();
    }
    else
    {
        const int n = dims();
        for (int dim = 0; dim < n; ++dim)
        {
            (*stds)[dim] = 0.0;
        }
    }
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

double gmm::Cluster::sample_probability(int sample) const
{
    const int n = dims();
    double prob = phi;
    for (int dim = 0; dim < n; ++dim)
    {
        prob *= dnorm(data(sample, dim), mu(dim, 0), (*stds)[dim]);
    }
    return prob;
}

double gmm::Cluster::sample_probability(int sample, const MatrixXd& cov_inv,
                                        const double cov_determinant) const
{
    // if (sample == 0)
    //     std::cerr << "<<<Data>>> " << data(sample).reshaped(1, dims()) << '\n';
    double prob;
    const int n = dims();
    (void)(n);
    (void)(dnorm);

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

    return prob;
}

namespace gmm
{
std::ostream& operator<<(std::ostream& os, const gmm::Cluster& clusterObj)
{
    os << "Cluster(id = " << clusterObj.idx << "): " << "data(" << clusterObj.data.rows() << ", "
       << clusterObj.data.cols() << ") \tmu(" << clusterObj.mu.rows() << ", "
       << clusterObj.mu.cols() << ") num_clusters = " << clusterObj.num_clusters
       << " dims = " << clusterObj.dims() << " samples = " << clusterObj.samples()
       << " clusters = " << clusterObj.clusters();
    return os;
}
}
