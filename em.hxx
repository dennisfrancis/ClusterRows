
#ifndef __CLUSTERROWS_EM__
#define __CLUSTERROWS_EM__

#include <cppu/unotype.hxx>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <math.h>

#include "logging.hxx"
#include "datatypes.hxx"

#define NUMITER 100
#define MAXEPOCHS 10
#define EPSILON 0.001

using com::sun::star::uno::Any;
using com::sun::star::uno::Sequence;

void performNoOpClustering(const Sequence<Sequence<Any>> &rDataArray,
                           const std::vector<DataType> &rColType,
                           const std::vector<std::pair<double, double>> &rFeatureScales,
                           std::vector<sal_Int32> &rClusterLabels,
                           std::vector<double> &rLabelConfidence,
                           sal_Int32 &rNumClusters);

void performEMClustering(const Sequence<Sequence<Any>> &rDataArray,
                         const std::vector<DataType> &rColType,
                         const std::vector<std::pair<double, double>> &rFeatureScales,
                         std::vector<sal_Int32> &rClusterLabels,
                         std::vector<double> &rLabelConfidence,
                         sal_Int32 &rNumClusters);

class GMM;

class GMMModel
{

public:
    GMMModel(sal_Int32 nNumClusters, const GMM *pTrainer);
    ~GMMModel() {}

    double Fit();
    void GetClusterLabels(std::vector<sal_Int32> &rClusterLabels,
                          std::vector<double> &rLabelConfidence,
                          sal_Int32 &rNumClusters);

private:
    double dnorm(double fX, double fMean, double fStd);
    double dnormNominal(double fX, double fMean, double fStd);
    void initParms();

    sal_Int32 mnNumClusters;
    const GMM *mpGMM;
    std::vector<std::vector<double>> maWeights;
    std::vector<double> maPhi;
    std::vector<std::vector<double>> maMeans;
    std::vector<std::vector<double>> maStd;
    std::vector<sal_Int32> maClusterLabels;
    std::vector<double> maLabelConfidence;
    double mfBICScore;
};

class GMM
{
    friend GMMModel;

public:
    GMM(const Sequence<Sequence<Any>> &rDataArray,
        const std::vector<DataType> &rColType,
        const std::vector<std::pair<double, double>> &rFeatureScales);
    ~GMM() {}

    void TrainModel(const std::vector<sal_Int32> &rNumClustersArray);
    void GetClusterLabels(std::vector<sal_Int32> &rClusterLabels,
                          std::vector<double> &rLabelConfidence,
                          sal_Int32 &rNumClusters);

private:
    sal_Int32 mnNumSamples;
    sal_Int32 mnNumDimensions;
    std::vector<std::vector<double>> maData;
    std::vector<sal_Int32> maNumClasses;
    std::unique_ptr<GMMModel> mpBestModel;
};

GMM::GMM(const Sequence<Sequence<Any>> &rDataArray,
         const std::vector<DataType> &rColType,
         const std::vector<std::pair<double, double>> &rFeatureScales)
{
    mnNumSamples = rDataArray.getLength();
    mnNumDimensions = rColType.size();
    writeLog("mnNumSamples = %d, mnNumDimensions = %d\n", mnNumSamples, mnNumDimensions);
    maData.resize(mnNumSamples);
    for (sal_Int32 nRowIdx = 0; nRowIdx < mnNumSamples; ++nRowIdx)
        maData[nRowIdx].resize(mnNumDimensions);

    maNumClasses.resize(mnNumDimensions);

    for (sal_Int32 nColIdx = 0; nColIdx < mnNumDimensions; ++nColIdx)
    {
        // Convert strings to numeric labels
        if (rColType[nColIdx] == DataType::STRING)
        {
            std::map<OUString, sal_Int32> aLabelMap;
            std::map<OUString, sal_Int32>::iterator aItr;
            OUString aStr;
            sal_Int32 nLabelIdx = 0;
            for (sal_Int32 nRowIdx = 0; nRowIdx < mnNumSamples; ++nRowIdx)
            {
                rDataArray[nRowIdx][nColIdx] >>= aStr;
                aItr = aLabelMap.find(aStr);
                if (aItr == aLabelMap.end())
                {
                    maData[nRowIdx][nColIdx] = static_cast<double>(nLabelIdx);
                    aLabelMap[aStr] = nLabelIdx++;
                }
                else
                    maData[nRowIdx][nColIdx] = static_cast<double>(aItr->second);
            }

            maNumClasses[nColIdx] = nLabelIdx; // Label count of nominal variable
        }
        else // Subtract feature mean and divide by feature std.
        {
            double fVal = 0.0;
            for (sal_Int32 nRowIdx = 0; nRowIdx < mnNumSamples; ++nRowIdx)
            {
                rDataArray[nRowIdx][nColIdx] >>= fVal;
                fVal = ((fVal - rFeatureScales[nColIdx].first) / rFeatureScales[nColIdx].second);
                maData[nRowIdx][nColIdx] = fVal;
            }

            maNumClasses[nColIdx] = 0; // Indicating that this feature is ordinal
        }
    }
}

void GMM::TrainModel(const std::vector<sal_Int32> &rNumClustersArray)
{
    std::unique_ptr<GMMModel> pModel;
    double fBestBIC = 9999999;
    sal_Int32 nBestNumClusters = 2;

    for (sal_Int32 nNumClusters : rNumClustersArray)
    {
        pModel.reset(new GMMModel(nNumClusters, this));
        const double fCurrBIC = pModel->Fit();
        if (fCurrBIC < fBestBIC)
        {
            fBestBIC = fCurrBIC;
            mpBestModel.reset(pModel.release());
            nBestNumClusters = nNumClusters;
        }
    }

    writeLog("\n########### Best model BIC score = %f, num clusters = %d\n", fBestBIC, nBestNumClusters);
}

void GMM::GetClusterLabels(std::vector<sal_Int32> &rClusterLabels,
                           std::vector<double> &rLabelConfidence,
                           sal_Int32 &rNumClusters)
{
    mpBestModel->GetClusterLabels(rClusterLabels, rLabelConfidence, rNumClusters);
}

GMMModel::GMMModel(sal_Int32 nNumClusters, const GMM *pTrainer) : mnNumClusters(nNumClusters),
                                                                  mpGMM(pTrainer),
                                                                  mfBICScore(9999999)
{
    maWeights.resize(mpGMM->mnNumSamples);
    for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
        maWeights[nSampleIdx].resize(mnNumClusters);

    maPhi.resize(mnNumClusters);
    double fPhi = (1.0 / static_cast<double>(mnNumClusters));
    for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
        maPhi[nClusterIdx] = fPhi;

    maMeans.resize(mnNumClusters);
    maStd.resize(mnNumClusters);
    for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
    {
        maMeans[nClusterIdx].resize(mpGMM->mnNumDimensions);
        maStd[nClusterIdx].resize(mpGMM->mnNumDimensions);
    }

    maClusterLabels.resize(mpGMM->mnNumSamples);
    maLabelConfidence.resize(mpGMM->mnNumSamples);
}

void GMMModel::initParms()
{
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // Select mnNumClusters data points at random from the samples to act as cluster centers.
    std::vector<sal_Int32> aSampleIndices(mpGMM->mnNumSamples);
    for (sal_Int32 nIdx = 0; nIdx < mpGMM->mnNumSamples; ++nIdx)
        aSampleIndices[nIdx] = nIdx;
    std::shuffle(aSampleIndices.begin(), aSampleIndices.end(), std::default_random_engine(seed));

    for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
    {
        sal_Int32 nRandIdx = aSampleIndices[nClusterIdx];
        maMeans[nClusterIdx] = mpGMM->maData[nRandIdx];
        maStd[nClusterIdx].assign(mpGMM->mnNumDimensions, 1.5);
    }
    // Do not init maClusterLabels or maLabelConfidence
    // as they are holding the best of all epochs.
}

double GMMModel::dnorm(double fX, double fMean, double fStd)
{
    const double fScale = 0.3989422804 / fStd;
    double fArg = (fX - fMean) / fStd;
    double fVal = exp(-0.5 * (fArg * fArg)) * fScale;
    return fVal;
}

double GMMModel::dnormNominal(double fX, double fMean, double fStd)
{
    const double fScale = 0.3989422804 / fStd;
    double fVar = (fStd * fStd);
    double fNum = (fX == fMean) ? 0.0 : 1.0;
    double fVal = exp(-0.5 * fNum / fVar) * fScale;
    return fVal;
}

double GMMModel::Fit()
{
    std::vector<double> aEpochLabelConfidence(mpGMM->mnNumSamples);
    std::vector<sal_Int32> aEpochClusterLabels(mpGMM->mnNumSamples);
    std::vector<double> aTmpLabelConfidence(mpGMM->mnNumSamples);
    std::vector<sal_Int32> aTmpClusterLabels(mpGMM->mnNumSamples);
    writeLog("\nFitting for #clusters = %d\n", mnNumClusters);
    for (int nEpochIdx = 0; nEpochIdx < MAXEPOCHS; ++nEpochIdx)
    {
        initParms();
        double fEpochBICScore = 9999999;
        writeLog("\n\tEpoch #%d : ", nEpochIdx);
        for (int nIter = 0; nIter < NUMITER; ++nIter)
        {
            // E step
            {
                double fBICScore = 0.0;
                for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                {
                    double fNormalizer = 0.0;
                    for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                    {
                        double fWeightVal = maPhi[nClusterIdx];
                        for (sal_Int32 nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                        {
                            if (mpGMM->maNumClasses[nDimIdx] == 0)
                                fWeightVal *= dnorm(
                                    mpGMM->maData[nSampleIdx][nDimIdx],
                                    maMeans[nClusterIdx][nDimIdx],
                                    maStd[nClusterIdx][nDimIdx]);
                            else
                                fWeightVal *= dnormNominal(
                                    mpGMM->maData[nSampleIdx][nDimIdx],
                                    maMeans[nClusterIdx][nDimIdx],
                                    maStd[nClusterIdx][nDimIdx]);
                        }

                        maWeights[nSampleIdx][nClusterIdx] = fWeightVal;

                        fNormalizer += fWeightVal;
                    }

                    // Find best cluster for sample nSampleIdx
                    double fBestClusterWeight = 0.0;
                    sal_Int32 nBestCluster = 0;

                    // Apply normalization factor to all elems of maWeights[nSampleIdx]
                    for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
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
                    fBICScore += (-log(abs(fBestClusterWeight)));
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
                for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fPhi = 0.0;
                    for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                        fPhi += maWeights[nSampleIdx][nClusterIdx];
                    fPhi /= static_cast<double>(mpGMM->mnNumSamples);
                    maPhi[nClusterIdx] = fPhi;
                }

                //Update maMeans
                for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fDen = (static_cast<double>(mpGMM->mnNumSamples) *
                                   maPhi[nClusterIdx]);
                    for (sal_Int32 nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                    {
                        sal_Int32 nNumClasses = mpGMM->maNumClasses[nDimIdx];
                        // Ordinal variable
                        if (nNumClasses == 0)
                        {
                            double fNum = 0.0;
                            for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                                fNum += (maWeights[nSampleIdx][nClusterIdx] * mpGMM->maData[nSampleIdx][nDimIdx]);
                            maMeans[nClusterIdx][nDimIdx] = (fNum / fDen);
                        }
                        else // Nominal variable
                        {
                            // Estimate the most probable label for this cluster.
                            double fBestLabelScore = 0.0;
                            double fBestLabel = 0;
                            for (sal_Int32 nLabel = 0; nLabel < nNumClasses; ++nLabel)
                            {
                                double fLabelScore = 0.0;
                                for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                                    if (nLabel == static_cast<sal_Int32>(mpGMM->maData[nSampleIdx][nDimIdx]))
                                        fLabelScore += maWeights[nSampleIdx][nClusterIdx];
                                if (fLabelScore > fBestLabelScore)
                                {
                                    fBestLabelScore = fLabelScore;
                                    fBestLabel = static_cast<double>(nLabel);
                                }
                            }

                            maMeans[nClusterIdx][nDimIdx] = fBestLabel;
                        }
                    }
                }

                // Update maStd
                for (sal_Int32 nClusterIdx = 0; nClusterIdx < mnNumClusters; ++nClusterIdx)
                {
                    double fDen = (static_cast<double>(mpGMM->mnNumSamples) *
                                   maPhi[nClusterIdx]);
                    for (sal_Int32 nDimIdx = 0; nDimIdx < mpGMM->mnNumDimensions; ++nDimIdx)
                    {
                        sal_Int32 nNumClasses = mpGMM->maNumClasses[nDimIdx];
                        const double fMean = maMeans[nClusterIdx][nDimIdx];
                        // Ordinal variable
                        if (nNumClasses == 0)
                        {
                            double fNum = 0.0;
                            for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                            {
                                const double fX = mpGMM->maData[nSampleIdx][nDimIdx];
                                fNum += (maWeights[nSampleIdx][nClusterIdx] * fX * fX);
                            }
                            maStd[nClusterIdx][nDimIdx] = sqrt((fNum / fDen) - (fMean * fMean));
                        }
                        else // Nominal variable
                        {
                            double fNum = 0.0;
                            for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
                            {
                                const double fX = mpGMM->maData[nSampleIdx][nDimIdx];
                                const double fDiff = (fX == fMean) ? 0.0 : 1.0;
                                fNum += (maWeights[nSampleIdx][nClusterIdx] * fDiff);
                            }
                            maStd[nClusterIdx][nDimIdx] = sqrt(fNum / fDen);
                        }
                    }
                }

            } // End of M step
        }     // End of one epoch

        if (fEpochBICScore < mfBICScore)
        {
            mfBICScore = fEpochBICScore;
            maClusterLabels = aEpochClusterLabels;
            maLabelConfidence = aEpochLabelConfidence;
            writeLog("\n\tThere is improvement in global BIC score, improved score = %f", mfBICScore);
        }

    } // End of epoch loop
    writeLog("\n\t**** Best BIC score over all epochs = %f\n", mfBICScore);
    return mfBICScore;
}

void GMMModel::GetClusterLabels(std::vector<sal_Int32> &rClusterLabels,
                                std::vector<double> &rLabelConfidence,
                                sal_Int32 &rNumClusters)
{
    for (sal_Int32 nSampleIdx = 0; nSampleIdx < mpGMM->mnNumSamples; ++nSampleIdx)
    {
        rClusterLabels[nSampleIdx] = maClusterLabels[nSampleIdx];
        rLabelConfidence[nSampleIdx] = maLabelConfidence[nSampleIdx];
    }
    rNumClusters = mnNumClusters;
}

// ----------------------------------------------------------------------------------------------------------

void performNoOpClustering(const Sequence<Sequence<Any>> &rDataArray,
                           const std::vector<DataType> &rColType,
                           const std::vector<std::pair<double, double>> &rFeatureScales,
                           std::vector<sal_Int32> &rClusterLabels,
                           std::vector<double> &rLabelConfidence,
                           sal_Int32 &rNumClusters)
{
    sal_Int32 nNumRows = rClusterLabels.size();
    sal_Int32 nClusterSize = static_cast<sal_Int32>((static_cast<double>(nNumRows) / rNumClusters) + 0.5);
    sal_Int32 nRowIdx = 0;
    for (sal_Int32 nClusterIdx = 0; (nClusterIdx < rNumClusters) && (nRowIdx < nNumRows); ++nClusterIdx)
    {
        for (sal_Int32 nMemberIdx = 0; (nMemberIdx < nClusterSize) && (nRowIdx < nNumRows); ++nMemberIdx)
        {
            rClusterLabels[nRowIdx] = nClusterIdx;
            rLabelConfidence[nRowIdx] = (static_cast<double>(nMemberIdx) / nClusterSize);
            ++nRowIdx;
        }
    }

    for (; nRowIdx < nNumRows; ++nRowIdx)
    {
        rClusterLabels[nRowIdx] = rClusterLabels[nRowIdx - 1];
        rLabelConfidence[nRowIdx] = rLabelConfidence[nRowIdx - 1];
    }
}

void performEMClustering(const Sequence<Sequence<Any>> &rDataArray,
                         const std::vector<DataType> &rColType,
                         const std::vector<std::pair<double, double>> &rFeatureScales,
                         std::vector<sal_Int32> &rClusterLabels,
                         std::vector<double> &rLabelConfidence,
                         sal_Int32 &rNumClusters)
{
    GMM aGMM(rDataArray, rColType, rFeatureScales);
    if (rNumClusters <= 1) // Auto computer optimum number of clusters
    {
        const std::vector<sal_Int32> aNumClustersArray = {2, 3, 4, 5};
        aGMM.TrainModel(aNumClustersArray);
    }
    else
    {
        const std::vector<sal_Int32> aNumClustersArray = {rNumClusters};
        aGMM.TrainModel(aNumClustersArray);
    }
    aGMM.GetClusterLabels(rClusterLabels, rLabelConfidence, rNumClusters);
}

#endif
