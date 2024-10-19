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

#include <gmm/data.hxx>

#include <Eigen/Dense>
#include <ostream>

namespace gmm
{
using namespace Eigen;
class Model;
class Data;

class Cluster
{
    const Data& data; // m x n
    MatrixXd mu;
    MatrixXd sigma;
    double phi;
    int num_clusters;
    int idx;

public:
    [[nodiscard]] int samples() const { return data.rows(); }
    [[nodiscard]] int dims() const { return data.cols(); }
    [[nodiscard]] int clusters() const { return num_clusters; }

    Cluster(int idx_, const Data& data_, int num_clusters_);
    void init(int use_sample);
    void init_cheat();

    void clear_mu_sigma();

    [[nodiscard]] MatrixXd inv_covar_matrix() const { return sigma.inverse(); }
    [[nodiscard]] double cov_determinant() const { return sigma.determinant(); }
    [[nodiscard]] double sample_probability(int sample, const MatrixXd& cov_inv,
                                            const double cov_determinant) const;

    friend class Model;
    friend std::ostream& operator<<(std::ostream&, const Cluster&);
};
}
