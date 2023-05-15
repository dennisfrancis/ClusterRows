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

#include "gmm/cluster.hxx"

#include <chrono>
#include <random>

gmm::Cluster::Cluster(int32_t id, const util::DataMatrix& data, const util::Matrix& weights)
    : m_id(id)
    , m_phi(1.0 / weights.rows())
    , m_mu(data.cols(), 1)
    , m_sigma(data.cols(), data.cols())
    , data(data)
    , weights(weights)
{
    init();
}

void gmm::Cluster::init()
{
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    // Initialize mu by taking mean of a random bunch of data samples.
    const int m = num_samples();
    const int n = num_dims();
    std::vector<int> sample_indices(m);
    for (int idx = 0; idx < m; ++idx)
        sample_indices[idx] = idx;
    std::shuffle(sample_indices.begin(), sample_indices.end(), std::default_random_engine(seed));

    int group_size = m / weights.rows();
    const int next = group_size * m_id;
    const int upper = std::min(next + group_size, m);
    group_size = upper - next + 1;
    for (int dim = 0; dim < n; ++dim)
        m_mu.at(dim, 0) = 0.0;
    for (int idx = next; idx < upper; ++idx)
    {
        for (int dim = 0; dim < n; ++dim)
        {
            if (idx == next)
                m_mu.at(dim, 0) = data[idx][dim] / group_size;
            else
                m_mu.at(dim, 0) += (data[idx][dim] / group_size);
        }
    }

    // Initialize covariance with identity matrix.
    m_sigma.set_identity();
}

int gmm::Cluster::num_samples() const { return data.rows(); }

int gmm::Cluster::num_dims() const { return data.cols(); }
