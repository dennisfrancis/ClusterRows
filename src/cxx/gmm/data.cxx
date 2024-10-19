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

#include <gmm/data.hxx>
#include <iostream>

#define DATA_NOOP 1

gmm::Data::Data(const Map<const MatrixXdRM>& data_)
    : _data{ data_ }
    , _mean{ data_.cols() }
    , _stdev{ data_.cols() }
{
    int n = _data.cols();
    int m = _data.rows();
    for (int dim = 0; dim < n; ++dim)
    {
        double& mean = _mean(dim);
        double& stdev = _stdev(dim);
        mean = 0;
        stdev = 0;
        for (int sample = 0; sample < m; ++sample)
        {
            double val = _data(sample, dim);
            double oldMean = mean;
            mean += (val - mean) / (sample + 1);
            stdev += (val - mean) * (val - oldMean);
        }

        // Store std-dev.
        stdev = std::sqrt(stdev / (m - 1));
    }
}

Eigen::MatrixXd gmm::Data::operator()(int sample) const
{
#ifndef DATA_NOOP
    return ((_data.row(sample).reshaped().array() - _mean) / _stdev);
#else
    return _data.row(sample);
#endif
}

double gmm::Data::operator()(int sample, int dim) const
{
#ifndef DATA_NOOP
    return (_data(sample, dim) - _mean(dim)) / _stdev(dim);
#else
    return _data(sample, dim);
#endif
}

void gmm::Data::transform(Eigen::ArrayXd& raw) const
{
#ifndef DATA_NOOP
    for (int dim = 0; dim < _mean.size(); ++dim)
    {
        raw(dim) = (raw(dim) - _mean(dim)) / _stdev(dim);
    }
#else
    (void)(raw);
#endif
}

void gmm::Data::display() const
{
    std::cerr << "Data : samples = " << _data.rows() << ", dims = " << _data.cols() << '\n';
    std::cerr << "First sample = ";
    for (int dim = 0; dim < _mean.size(); ++dim)
    {
        std::cerr << _data(0, dim) << " ";
    }
    std::cerr << '\n';
    std::cerr << "_mean = ";
    for (int dim = 0; dim < _mean.size(); ++dim)
    {
        std::cerr << _mean[dim] << " ";
    }
    std::cerr << '\n';
    std::cerr << "_stdev = ";
    for (int dim = 0; dim < _mean.size(); ++dim)
    {
        std::cerr << _stdev[dim] << " ";
    }
    std::cerr << "\n";
}