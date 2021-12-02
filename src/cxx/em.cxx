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

extern "C" int CR_DLLPUBLIC_EXPORT gmm(const double* array, int rows, int cols, int numClusters,
                                       int numEpochs, int numIterations, int* clusterLabels,
                                       double* labelConfidence)
{
    if (!array || !clusterLabels || !labelConfidence || rows < 10)
        return -1;

    em::DataMatrix mat(array, rows, cols);

    em::GMM aGMM(array, rows, cols, numEpochs, numIterations);
    if (numClusters <= 1) // Auto computer optimum number of clusters
    {
        const std::vector<int> aNumClustersArray = { 2, 3, 4, 5 };
        aGMM.TrainModel(aNumClustersArray);
    }
    else
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
        pModel.reset(new GMMModel(numClusters, this, mnNumEpochs, mnNumIter));
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

em::GMMModel::GMMModel(const int numClusters, const GMM* pTrainer, int numEpochs, int numIter)
    : mnNumClusters(numClusters)
    , mpGMM(pTrainer)
    , mfBICScore(9999999)
    , mnNumEpochs(numEpochs)
    , mnNumIter(numIter)
{
    maWeights.resize(mpGMM->mnNumSamples);
    for (int sampleIdx = 0; sampleIdx < mpGMM->mnNumSamples; ++sampleIdx)
        maWeights[sampleIdx].resize(mnNumClusters);

    maPhi.resize(mnNumClusters);
    double fPhi = (1.0 / static_cast<double>(mnNumClusters));
    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
        maPhi[clusterIdx] = fPhi;

    maMeans.resize(mnNumClusters);
    maStd.resize(mnNumClusters);
    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
    {
        maMeans[clusterIdx].resize(mpGMM->mnNumDimensions);
        maStd[clusterIdx].resize(mpGMM->mnNumDimensions);
    }

    maClusterLabels.resize(mpGMM->mnNumSamples);
    maLabelConfidence.resize(mpGMM->mnNumSamples);
}

void em::GMMModel::initParms()
{
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // Select mnNumClusters data points at random from the samples to act as cluster centers.
    std::vector<int> sampleIndices(mpGMM->mnNumSamples);
    for (int idx = 0; idx < mpGMM->mnNumSamples; ++idx)
        sampleIndices[idx] = idx;
    std::shuffle(sampleIndices.begin(), sampleIndices.end(), std::default_random_engine(seed));

    for (int clusterIdx = 0; clusterIdx < mnNumClusters; ++clusterIdx)
    {
        int randIdx = sampleIndices[clusterIdx];
        for (int dim = 0; dim < mpGMM->mnNumDimensions; ++dim)
            maMeans[clusterIdx][dim] = mpGMM->getNormalized(randIdx, dim);
        maStd[clusterIdx].assign(mpGMM->mnNumDimensions, 1.5);
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
    std::vector<double> aEpochLabelConfidence(mpGMM->mnNumSamples);
    std::vector<int> aEpochClusterLabels(mpGMM->mnNumSamples);
    std::vector<double> aTmpLabelConfidence(mpGMM->mnNumSamples);
    std::vector<int> aTmpClusterLabels(mpGMM->mnNumSamples);
    writeLog("\nFitting for #clusters = %d\n", mnNumClusters);
    for (int nEpochIdx = 0; nEpochIdx < mnNumEpochs; ++nEpochIdx)
    {
        initParms();
        double fEpochBICScore = 9999999;
        writeLog("\n\tEpoch #%d : ", nEpochIdx);
        for (int nIter = 0; nIter < mnNumIter; ++nIter)
        {
            // E step
            {
                double fBICScore = 0.0;
                for (int nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                {
                    double fNormalizer = 0.0;
                    for (int nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                    {
                        double fWeightVal = maPhi[nClusterIdx];
                        for (int nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                        {
                            fWeightVal *= dnorm(mpGMM->getNormalized(nSampleIdx, nDimIdx),
                                                maMeans[nClusterIdx][nDimIdx],
                                                maStd[nClusterIdx][nDimIdx]);
                        }

                        maWeights[nSampleIdx][nClusterIdx] = fWeightVal;

                        fNormalizer += fWeightVal;
                    }

                    // Find best cluster for sample nSampleIdx
                    double fBestClusterWeight = 0.0;
                    int nBestCluster = 0;

                    // Apply normalization factor to all elems of maWeights[nSampleIdx]
                    for (int nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                    {
                        double fWt = maWeights[nSampleIdx][nClusterIdx];
                        fWt /= fNormalizer;
                        maWeights[nSampleIdx][nClusterIdx] = fWt;
                        if (fWt > fBestClusterWeight)
                        {
                            fBestClusterWeight = fWt;
                            nBestCluster = nClusterIdx;
                        }
                    }
                    aTmpClusterLabels[nSampleIdx] = nBestCluster;
                    aTmpLabelConfidence[nSampleIdx] = fBestClusterWeight;
                    fBICScore += (-std::log(std::abs(fBestClusterWeight)));
                }

                writeLog("%f, ", fBICScore);
                if (fEpochBICScore > fBICScore && ((fEpochBICScore - fBICScore) > EPSILON))
                {
                    // There is improvement in BIC score in this epoch.
                    fEpochBICScore = fBICScore;
                    aEpochClusterLabels = aTmpClusterLabels;
                    aEpochLabelConfidence = aTmpLabelConfidence;
                }
                else
                {
                    writeLog("\n\tBest BIC score of epoch#%d = %f", nEpochIdx, fEpochBICScore);
                    break;
                }

            } // End of E step

            // M step
            {
                // Update maPhi
                for (int nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fPhi = 0.0;
                    for (int nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                        fPhi += maWeights[nSampleIdx][nClusterIdx];
                    fPhi /= static_cast<double>(mpGMM->mnNumSamples);
                    maPhi[nClusterIdx] = fPhi;
                }

                //Update maMeans
                for (int nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fDen = (static_cast<double>(mpGMM->mnNumSamples) * maPhi[nClusterIdx]);
                    for (int nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                    {
                        double fNum = 0.0;
                        for (int nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                            fNum += (maWeights[nSampleIdx][nClusterIdx]
                                     * mpGMM->getNormalized(nSampleIdx, nDimIdx));
                        maMeans[nClusterIdx][nDimIdx] = (fNum / fDen);
                    }
                }

                // Update maStd
                for (int nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fDen = (static_cast<double>(mpGMM->mnNumSamples) * maPhi[nClusterIdx]);
                    for (int nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                    {
                        const double fMean = maMeans[nClusterIdx][nDimIdx];
                        double fNum = 0.0;
                        for (int nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                        {
                            const double fX = mpGMM->getNormalized(nSampleIdx, nDimIdx);
                            fNum += (maWeights[nSampleIdx][nClusterIdx] * fX * fX);
                        }
                        maStd[nClusterIdx][nDimIdx] = std::sqrt((fNum / fDen) - (fMean * fMean));
                    }
                }

            } // End of M step
        } // End of one epoch

        if (fEpochBICScore < mfBICScore)
        {
            mfBICScore = fEpochBICScore;
            maClusterLabels = aEpochClusterLabels;
            maLabelConfidence = aEpochLabelConfidence;
            writeLog("\n\tThere is improvement in global BIC score, improved score = %f",
                     mfBICScore);
        }

    } // End of epoch loop
    writeLog("\n\t**** Best BIC score over all epochs = %f\n", mfBICScore);
    return mfBICScore;
}

void em::GMMModel::GetClusterLabels(int* clusterLabels, double* labelConfidence)
{
    for (int sampleIdx = 0; sampleIdx < mpGMM->mnNumSamples; ++sampleIdx)
    {
        clusterLabels[sampleIdx] = maClusterLabels[sampleIdx];
        labelConfidence[sampleIdx] = maLabelConfidence[sampleIdx];
    }
}
