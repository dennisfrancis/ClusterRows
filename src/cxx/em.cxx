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

#include <em.h>
#include <model.hxx>
#include <legacy_gmm.hxx>

namespace
{
void fillConstLabel(int label, double confidence, int rows, int* clusterLabels,
                    double* labelConfidence)
{
    if (!clusterLabels || !labelConfidence)
        return;

    for (int idx = 0; idx < rows; ++idx)
    {
        clusterLabels[idx] = label;
        labelConfidence[idx] = confidence;
    }
}

}

extern "C" int CR_DLLPUBLIC_EXPORT gmmMain(const double* array, int rows, int cols, int numClusters,
                                           int numEpochs, int numIterations, int* clusterLabels,
                                           double* labelConfidence, int fullGMM)
{
    if (!array || !clusterLabels || !labelConfidence)
        return -1;

    if (numClusters == 1)
    {
        fillConstLabel(0, 1, rows, clusterLabels, labelConfidence);
        return 0;
    }

    if (rows < 10)
    {
        fillConstLabel(-1, 0, rows, clusterLabels, labelConfidence);
        return 0;
    }

    if (fullGMM)
    {
        bool autoMode{ numClusters <= 0 };
        int min_clusters = autoMode ? 2 : numClusters;
        int max_clusters = autoMode ? 5 : numClusters;
        gmm::GMM trainer{ array,        rows,      cols,          min_clusters,
                          max_clusters, numEpochs, numIterations, bool(fullGMM) };
        trainer.fit();
        trainer.get_labels(clusterLabels, labelConfidence);
    }
    else
    {
        util::DataMatrix mat(array, rows, cols);

        em::GMM gmm(array, rows, cols, numEpochs, numIterations);
        if (numClusters <= 0) // Auto computer optimum number of clusters
        {
            const std::vector<int> numClustersArray = { 2, 3, 4, 5 };
            gmm.TrainModel(numClustersArray);
        }
        else // numClusters > 1
        {
            const std::vector<int> numClustersArray = { numClusters };
            gmm.TrainModel(numClustersArray);
        }

        gmm.GetClusterLabels(clusterLabels, labelConfidence);
    }

    return 0;
}
