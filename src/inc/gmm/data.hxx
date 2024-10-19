/*
* ClusterRows
* Copyright (c) 2024 Dennis Francis <dennisfrancis.in@gmail.com>
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

#include "Eigen/Core"
#include <Eigen/Dense>

namespace gmm
{

using namespace Eigen;
using MatrixXdRM = Matrix<double, Dynamic, Dynamic, RowMajor>;

class Data
{
    const Map<const MatrixXdRM>& _data;
    // To store global mean and std.dev of the data.
    ArrayXd _mean;
    ArrayXd _stdev; // diagonal elements only.

public:
    Data(const Map<const MatrixXdRM>& data_);
    MatrixXd operator()(int sample) const;
    double operator()(int sample, int dim) const;
    int rows() const { return _data.rows(); }
    int cols() const { return _data.cols(); }
    void transform(ArrayXd& raw) const;
    void display() const;
};

}