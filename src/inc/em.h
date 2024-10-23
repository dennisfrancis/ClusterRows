/*
 * ClusterRows
 * Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
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
#include "macros.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief computes cluster assignments for each row of data according to gaussian mixture model.
    /// @param array input matrix stored in column major form.
    /// @param rows number of rows of the input matrix.
    /// @param cols number of columns of the input matrix.
    /// @param numClusters desired number of clusters (It will auto compute this if 0 is provided).
    /// @param numEpochs desired number of epochs.
    /// @param numIterations maximum number of iterations in each epoch.
    /// @param clusterLabels output array to put each row's cluster assignment label.
    /// @param labelConfidence output array to store confidence score of each cluster assignment.
    /// @param fullGMM specifies whether to perform a full covariance matrix GMM or not.
    /// @return 0 on success and -1 on failure.
    int CR_DLLPUBLIC_EXPORT gmmMain(const double* array, int rows, int cols, int numClusters,
                                    int numEpochs, int numIterations, int* clusterLabels,
                                    double* labelConfidence, int fullGMM);

#ifdef __cplusplus
}
#endif
