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

#include "gmm/model.hxx"

gmm::Model::Model(const util::DataMatrix& data, int num_clusters)
    : m_weights(num_clusters, data.rows())
    , data(data)
    , num_clusters(num_clusters)
{
    init();
}

void gmm::Model::init()
{
    double prob = 1.0 / num_clusters;
    m_weights.set(prob);
}
