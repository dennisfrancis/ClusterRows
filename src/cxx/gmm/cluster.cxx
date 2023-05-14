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
    // TODO: Implement me.
}

int gmm::Cluster::num_samples() const { return data.rows(); }

int gmm::Cluster::num_dims() const { return data.cols(); }
