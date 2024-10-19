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

#include "Eigen/Core"
#include <Eigen/Dense>

#include <memory>
#include <vector>

namespace gmm
{

using namespace Eigen;
class Cluster;

class Model
{
public:
    Model(const Map<const MatrixXdRM>& data, int num_clusters);

    [[nodiscard]] int clusters() const { return num_clusters; }
    [[nodiscard]] int samples() const { return data.rows(); }
    [[nodiscard]] int dims() const { return data.cols(); }

    double fit(int num_epochs, int num_iterations);
    void get_labels(int* labels, double* confidence_scores) const;

private:
    [[nodiscard]] double run_epoch(int num_iterations, MatrixXd& epoch_weights,
                                   std::vector<Cluster>& epoch_clusters) const;
    [[nodiscard]] double compute_expectation(MatrixXd& epoch_weights,
                                             const std::vector<Cluster>& epoch_clusters) const;
    void maximize_likelihood(const MatrixXd& epoch_weights,
                             std::vector<Cluster>& epoch_clusters) const;

private:
    MatrixXd weights; // shape is c x m
    Data data; // shape is m x n
    const int num_clusters;
};

class GMM
{
public:
    GMM(const double* data_, int rows_, int cols_, int min_clusters_, int max_clusters_,
        int num_epochs_, int num_iterations_);
    void fit();
    void get_labels(int* labels, double* confidence_scores) const;

private:
    const Map<const MatrixXdRM> data;
    std::unique_ptr<Model> best_model;
    const int min_clusters;
    const int max_clusters;
    const int num_epochs;
    const int num_iterations;
};

}
