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

#if defined(_WIN32) || defined(WIN32)
#define CR_DLLPUBLIC_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define CR_DLLPUBLIC_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    int CR_DLLPUBLIC_EXPORT gmm(const double* array, int rows, int cols, int numClusters,
                                int numEpochs, int numIterations, int* clusterLabels,
                                double* labelConfidence);

#ifdef __cplusplus
}
#endif
