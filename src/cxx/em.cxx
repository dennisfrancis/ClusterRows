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
#include <algorithm>
#include <random>
#include <cstring>

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

    em::GMM aGMM(array, rows, cols, numEpochs, numIterations);
    if (numClusters <= 0) // Auto computer optimum number of clusters
    {
        const std::vector<int> aNumClustersArray = { 2, 3, 4, 5 };
        aGMM.TrainModel(aNumClustersArray);
    }
    else // numClusters > 1
    {
        const std::vector<int> aNumClustersArray = { numClusters };
        aGMM.TrainModel(aNumClustersArray);
    }

    aGMM.GetClusterLabels(clusterLabels, labelConfidence);

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
        maStds[dim] = 0;
        maMeans[dim] = 0;
    }

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
    int bestNumClusters = 2;

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
    : mnNumClusters(numClusters)
    , mrGMM(rTrainer)
    , mfBICScore(9999999)
    , mnNumEpochs(numEpochs)
    , mnNumIter(numIter)
{
    maWeights.resize(mrGMM.mnNumSamples);
    for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
        maWeights[sampleIdx].resize(mnNumClusters);

    maPhi.resize(mnNumClusters);
    double fPhi = (1.0 / static_cast<double>(mnNumClusters));
    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
        maPhi[clusterIdx] = fPhi;

    maMeans.resize(mnNumClusters);
    maStd.resize(mnNumClusters);
    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
    {
        maMeans[clusterIdx].resize(mrGMM.mnNumDimensions);
        maStd[clusterIdx].resize(mrGMM.mnNumDimensions);
    }

    maClusterLabels.resize(mrGMM.mnNumSamples);
    maLabelConfidence.resize(mrGMM.mnNumSamples);
}

void em::GMMModel::initParms()
{
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // Select mnNumClusters data points at random from the samples to act as cluster centers.
    std::vector<int> sampleIndices(mrGMM.mnNumSamples);
    for (int idx = 0; idx < mrGMM.mnNumSamples; ++idx)
        sampleIndices[idx] = idx;
    std::shuffle(sampleIndices.begin(), sampleIndices.end(), std::default_random_engine(seed));

    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
    {
        int randIdx = sampleIndices[clusterIdx];
        for (int dim = 0; dim < mrGMM.mnNumDimensions; ++dim)
            maMeans[clusterIdx][dim] = mrGMM.getNormalized(randIdx, dim);
        maStd[clusterIdx].assign(mrGMM.mnNumDimensions, 1.5);
    }
    // Do not init maClusterLabels or maLabelConfidence
    // as they are holding the best of all epochs.
}

double em::GMMModel::dnorm(double x, double mean, double stddev)
{
    const double scale = 0.3989422804 / stddev;
    double arg = (x - mean) / stddev;
    return exp(-0.5 * (arg * arg)) * scale;
}

double em::GMMModel::Fit()
{
    std::vector<double> epochLabelConfidence(mrGMM.mnNumSamples);
    std::vector<int> epochClusterLabels(mrGMM.mnNumSamples);
    std::vector<double> tmpLabelConfidence(mrGMM.mnNumSamples);
    std::vector<int> tmpClusterLabels(mrGMM.mnNumSamples);
    writeLog("\nFitting for #clusters = %d\n", mnNumClusters);
    for (int epochIdx = 0; epochIdx < mnNumEpochs; ++epochIdx)
    {
        initParms();
        double epochBICScore = 9999999;
        writeLog("\n\tEpoch #%d : ", epochIdx);
        for (int iter = 0; iter < mnNumIter; ++iter)
        {
            // E step
            {
                double BICScore = 0.0;
                for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
                {
                    double normalizer = 0.0;
                    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
                    {
                        double weightVal = maPhi[clusterIdx];
                        for (int dimIdx = 0; dimIdx < mrGMM.mnNumDimensions; ++dimIdx)
                        {
                            weightVal
                                *= dnorm(mrGMM.getNormalized(sampleIdx, dimIdx),
                                         maMeans[clusterIdx][dimIdx], maStd[clusterIdx][dimIdx]);
                        }

                        maWeights[sampleIdx][clusterIdx] = weightVal;

                        normalizer += weightVal;
                    }

                    // Find best cluster for sample nSampleIdx
                    double bestClusterWeight = 0.0;
                    int bestCluster = 0;

                    // Apply normalization factor to all elems of maWeights[nSampleIdx]
                    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
                    {
                        double wt = maWeights[sampleIdx][clusterIdx];
                        wt /= normalizer;
                        maWeights[sampleIdx][clusterIdx] = wt;
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
                    writeLog("\n\tBest BIC score of epoch#%d = %f", epochIdx, epochBICScore);
                    break;
                }

            } // End of E step

            // M step
            {
                // Update maPhi
                for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
                {
                    double phi = 0.0;
                    for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
                        phi += maWeights[sampleIdx][clusterIdx];
                    phi /= static_cast<double>(mrGMM.mnNumSamples);
                    maPhi[clusterIdx] = phi;
                }

                //Update maMeans
                for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
                {
                    double den = (static_cast<double>(mrGMM.mnNumSamples) * maPhi[clusterIdx]);
                    for (int dimIdx = 0; dimIdx < mrGMM.mnNumDimensions; ++dimIdx)
                    {
                        double num = 0.0;
                        for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
                            num += (maWeights[sampleIdx][clusterIdx]
                                    * mrGMM.getNormalized(sampleIdx, dimIdx));
                        maMeans[clusterIdx][dimIdx] = (num / den);
                    }
                }

                // Update maStd
                for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
                {
                    double den = (static_cast<double>(mrGMM.mnNumSamples) * maPhi[clusterIdx]);
                    for (int dimIdx = 0; dimIdx < mrGMM.mnNumDimensions; ++dimIdx)
                    {
                        const double mean = maMeans[clusterIdx][dimIdx];
                        double num = 0.0;
                        for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
                        {
                            const double x = mrGMM.getNormalized(sampleIdx, dimIdx);
                            num += (maWeights[sampleIdx][clusterIdx] * x * x);
                        }
                        maStd[clusterIdx][dimIdx] = std::sqrt((num / den) - (mean * mean));
                    }
                }

            } // End of M step
        } // End of one epoch

        if (epochBICScore < mfBICScore)
        {
            mfBICScore = epochBICScore;
            maClusterLabels = epochClusterLabels;
            maLabelConfidence = epochLabelConfidence;
            writeLog("\n\tThere is improvement in global BIC score, improved score = %f",
                     mfBICScore);
        }

    } // End of epoch loop
    writeLog("\n\t**** Best BIC score over all epochs = %f\n", mfBICScore);
    return mfBICScore;
}

void em::GMMModel::GetClusterLabels(int* clusterLabels, double* labelConfidence)
{
    for (int sampleIdx = 0; sampleIdx < mrGMM.mnNumSamples; ++sampleIdx)
    {
        clusterLabels[sampleIdx] = maClusterLabels[sampleIdx];
        labelConfidence[sampleIdx] = maLabelConfidence[sampleIdx];
    }
}
