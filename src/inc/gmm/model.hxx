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

#pragma once

#include "matrix.hxx"
#include "datamatrix.hxx"

namespace gmm
{
class Model
{
public:
    Model(const util::DataMatrix& data, int num_clusters);

    [[nodiscard]] int cluster_count() const { return num_clusters; }
    [[nodiscard]] int sample_count() const { return data.rows(); }
    [[nodiscard]] int dim_count() const { return data.cols(); }

private:
    void init();

private:
    const util::Matrix m_weights; // shape is c x m
    const util::DataMatrix& data; // shape is m x n
    const int num_clusters;
};

}
