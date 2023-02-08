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

#include "../inc/em.hxx"
#include "../inc/logging.hxx"

#include <cmath>
#include <chrono>
#include <random>
#include <cstring>
#include <algorithm>

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

extern "C" int CR_DLLPUBLIC_EXPORT gmm(const double* array, int rows, int cols, int numClusters,
                                       int numEpochs, int numIterations, int* clusterLabels,
                                       double* labelConfidence)
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

    em::DataMatrix mat(array, rows, cols);

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

    return 0;
}

// em::GMM

em::GMM::GMM(const double* pRows, int nRows, int nCols, int nNumEpochs, int nNumIter)
    : mnNumSamples(nRows)
    , mnNumDimensions(nCols)
    , maData(pRows, nRows, nCols)
    , mnNumEpochs(nNumEpochs)
    , mnNumIter(nNumIter)
    , maStds(mnNumDimensions)
    , maMeans(mnNumDimensions)
{
    writeLog("mnNumSamples = %d, mnNumDimensions = %d\n", mnNumSamples, mnNumDimensions);
    computeStats();
}

void em::GMM::computeStats()
{
    for (int dim = 0; dim < mnNumDimensions; ++dim)
    {
        double& mean = maMeans[dim];
        double& var = maStds[dim];
        mean = 0;
        var = 0;
        for (int si = 0; si < mnNumSamples; ++si)
        {
            double val = maData[si][dim];
            double oldMean = mean;
            mean += (val - mean) / (si + 1);
            var += (val - mean) * (val - oldMean);
        }

        // Store std-dev.
        var = std::sqrt(var / (mnNumSamples - 1));
    }
}

double em::GMM::getNormalized(int row, int col) const
{
    return (maData[row][col] - maMeans[col]) / maStds[col];
}

void em::GMM::TrainModel(const std::vector<int>& numClustersArray)
{
    std::unique_ptr<GMMModel> pModel;
    double bestBIC = 9999999;
    int bestNumClusters = 1;

    for (int numClusters : numClustersArray)
    {
        pModel.reset(new GMMModel(numClusters, *this, mnNumEpochs, mnNumIter));
        const double currBIC = pModel->Fit();
        if (currBIC < bestBIC)
        {
            bestBIC = currBIC;
            mpBestModel.reset(pModel.release());
            bestNumClusters = numClusters;
        }
    }

    writeLog("\nBest model BIC score = %f, num clusters = %d\n", bestBIC, bestNumClusters);
}

void em::GMM::GetClusterLabels(int* clusterLabels, double* labelConfidence)
{
    mpBestModel->GetClusterLabels(clusterLabels, labelConfidence);
}

// em::GMMModel

em::GMMModel::GMMModel(const int numClusters, const GMM& rTrainer, int numEpochs, int numIter)
    : m_numClusters(numClusters)
    , m_rGMM(rTrainer)
    , m_BICScore(9999999)
    , m_numEpochs(numEpochs)
    , m_numIter(numIter)
{
    m_weights.resize(m_rGMM.mnNumSamples);
    for (int sampleIdx = 0; sampleIdx < m_rGMM.mnNumSamples; ++sampleIdx)
        m_weights[sampleIdx].resize(m_numClusters);

    m_phi.resize(m_numClusters);
    double phi = (1.0 / static_cast<double>(m_numClusters));
    std::fill_n(m_phi.begin(), m_numClusters, phi);

    m_means.resize(m_numClusters);
    m_std.resize(m_numClusters);
    for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
    {
        m_means[clusterIdx].resize(m_rGMM.mnNumDimensions);
        m_std[clusterIdx].resize(m_rGMM.mnNumDimensions);
    }

    m_clusterLabels.resize(m_rGMM.mnNumSamples);
    m_labelConfidence.resize(m_rGMM.mnNumSamples);
}

void em::GMMModel::initParms()
{
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // Select mnNumClusters data points at random from the samples to act as cluster centers.
    std::vector<int> sampleIndices(m_rGMM.mnNumSamples);
    for (int idx = 0; idx < m_rGMM.mnNumSamples; ++idx)
        sampleIndices[idx] = idx;
    std::shuffle(sampleIndices.begin(), sampleIndices.end(), std::default_random_engine(seed));

    for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
    {
        int randIdx = sampleIndices[clusterIdx];
        for (int dim = 0; dim < m_rGMM.mnNumDimensions; ++dim)
            m_means[clusterIdx][dim] = m_rGMM.getNormalized(randIdx, dim);
        m_std[clusterIdx].assign(m_rGMM.mnNumDimensions, 1.5);
    }
    // Do not init maClusterLabels or maLabelConfidence
    // as they are holding the best of all epochs.
}

static const double normDistScale = 1.0 / std::sqrt(2 * M_PI);

// Density of normal distribution at X = x with given mean and stddev
double em::GMMModel::dnorm(double x, double mean, double stddev)
{
    const double scale = normDistScale / stddev;
    double arg = (x - mean) / stddev;
    return exp(-0.5 * (arg * arg)) * scale;
}

double em::GMMModel::Fit()
{
    std::vector<double> epochLabelConfidence(m_rGMM.mnNumSamples);
    std::vector<int> epochClusterLabels(m_rGMM.mnNumSamples);
    std::vector<double> tmpLabelConfidence(m_rGMM.mnNumSamples);
    std::vector<int> tmpClusterLabels(m_rGMM.mnNumSamples);
    writeLog("\nFitting for #clusters = %d\n", m_numClusters);
    for (int epochIdx = 0; epochIdx < m_numEpochs; ++epochIdx)
    {
        double epochBICScore = runEpoch(epochIdx, epochLabelConfidence, epochClusterLabels,
                                        tmpLabelConfidence, tmpClusterLabels);
        if (epochBICScore < m_BICScore)
        {
            m_BICScore = epochBICScore;
            m_clusterLabels = epochClusterLabels;
            m_labelConfidence = epochLabelConfidence;
            writeLog("\n\tThere is improvement in global BIC score, improved score = %f",
                     m_BICScore);
        }

    } // End of epoch loop
    writeLog("\n\t**** Best BIC score over all epochs = %f\n", m_BICScore);
    return m_BICScore;
}

double em::GMMModel::runEpoch(int epochIndex, std::vector<double>& epochLabelConfidence,
                              std::vector<int>& epochClusterLabels,
                              std::vector<double>& tmpLabelConfidence,
                              std::vector<int>& tmpClusterLabels)
{
    initParms();
    double epochBICScore = 9999999;
    writeLog("\n\tEpoch #%d : ", epochIndex);
    for (int iter = 0; iter < m_numIter; ++iter)
    {
        // E step
        {
            double BICScore = 0.0;
            for (int sampleIdx = 0; sampleIdx < m_rGMM.mnNumSamples; ++sampleIdx)
            {
                double normalizer = 0.0;
                for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
                {
                    double weightVal = m_phi[clusterIdx];
                    for (int dimIdx = 0; dimIdx < m_rGMM.mnNumDimensions; ++dimIdx)
                    {
                        weightVal *= dnorm(m_rGMM.getNormalized(sampleIdx, dimIdx),
                                           m_means[clusterIdx][dimIdx], m_std[clusterIdx][dimIdx]);
                    }

                    m_weights[sampleIdx][clusterIdx] = weightVal;

                    normalizer += weightVal;
                }

                // Find best cluster for sample nSampleIdx
                double bestClusterWeight = 0.0;
                int bestCluster = 0;

                // Apply normalization factor to all elems of maWeights[nSampleIdx]
                for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
                {
                    double wt = m_weights[sampleIdx][clusterIdx];
                    wt /= normalizer;
                    m_weights[sampleIdx][clusterIdx] = wt;
                    if (wt > bestClusterWeight)
                    {
                        bestClusterWeight = wt;
                        bestCluster = clusterIdx;
                    }
                }
                tmpClusterLabels[sampleIdx] = bestCluster;
                tmpLabelConfidence[sampleIdx] = bestClusterWeight;
                BICScore += (-std::log(std::abs(bestClusterWeight)));
            }

            writeLog("%f, ", BICScore);
            if (epochBICScore > BICScore && ((epochBICScore - BICScore) > EPSILON))
            {
                // There is improvement in BIC score in this epoch.
                epochBICScore = BICScore;
                epochClusterLabels = tmpClusterLabels;
                epochLabelConfidence = tmpLabelConfidence;
            }
            else
            {
                writeLog("\n\tBest BIC score of epoch#%d = %f", epochIndex, epochBICScore);
                break;
            }

        } // End of E step

        // M step
        {
            // Update maPhi
            for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
            {
                double phi = 0.0;
                for (int sampleIdx = 0; sampleIdx < m_rGMM.mnNumSamples; ++sampleIdx)
                    phi += m_weights[sampleIdx][clusterIdx];
                phi /= static_cast<double>(m_rGMM.mnNumSamples);
                m_phi[clusterIdx] = phi;
            }

            //Update maMeans
            for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
            {
                double den = (static_cast<double>(m_rGMM.mnNumSamples) * m_phi[clusterIdx]);
                for (int dimIdx = 0; dimIdx < m_rGMM.mnNumDimensions; ++dimIdx)
                {
                    double num = 0.0;
                    for (int sampleIdx = 0; sampleIdx < m_rGMM.mnNumSamples; ++sampleIdx)
                        num += (m_weights[sampleIdx][clusterIdx]
                                * m_rGMM.getNormalized(sampleIdx, dimIdx));
                    m_means[clusterIdx][dimIdx] = (num / den);
                }
            }

            // Update maStd
            for (int clusterIdx = 0; clusterIdx < m_numClusters; ++clusterIdx)
            {
                double den = (static_cast<double>(m_rGMM.mnNumSamples) * m_phi[clusterIdx]);
                for (int dimIdx = 0; dimIdx < m_rGMM.mnNumDimensions; ++dimIdx)
                {
                    const double mean = m_means[clusterIdx][dimIdx];
                    double num = 0.0;
                    for (int sampleIdx = 0; sampleIdx < m_rGMM.mnNumSamples; ++sampleIdx)
                    {
                        const double x = m_rGMM.getNormalized(sampleIdx, dimIdx);
                        num += (m_weights[sampleIdx][clusterIdx] * x * x);
                    }
                    m_std[clusterIdx][dimIdx] = std::sqrt((num / den) - (mean * mean));
                }
            }

        } // End of M step
    } // End of one epoch

    return epochBICScore;
}

void em::GMMModel::GetClusterLabels(int* clusterLabels, double* labelConfidence)
{
    std::copy_n(m_clusterLabels.begin(), m_rGMM.mnNumSamples, clusterLabels);
    std::copy_n(m_labelConfidence.begin(), m_rGMM.mnNumSamples, labelConfidence);
}
