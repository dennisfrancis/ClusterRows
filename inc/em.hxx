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

#include <cppu/unotype.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>

#include <vector>

#include "logging.hxx"
#include "datatypes.hxx"

#define NUMITER 100
#define MAXEPOCHS 10
#define EPSILON 0.001

using com::sun::star::uno::Any;
using com::sun::star::uno::Sequence;

namespace em
{
void performNoOpClustering(
    const com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    const std::vector<DataType>& rColType,
    const std::vector<std::pair<double, double>>& rFeatureScales,
    std::vector<sal_Int32>& rClusterLabels, std::vector<double>& rLabelConfidence,
    sal_Int32& rNumClusters);

void performEMClustering(
    const com::sun::star::uno::Sequence<com::sun::star::uno::Sequence<com::sun::star::uno::Any>>&
        rDataArray,
    const std::vector<DataType>& rColType,
    const std::vector<std::pair<double, double>>& rFeatureScales,
    std::vector<sal_Int32>& rClusterLabels, std::vector<double>& rLabelConfidence,
    sal_Int32& rNumClusters, sal_Int32 nNumEpochs = static_cast<sal_Int32>(MAXEPOCHS),
    sal_Int32 nNumIter = static_cast<sal_Int32>(NUMITER));

class GMM;

class GMMModel
{
public:
    GMMModel(sal_Int32 nNumClusters, const GMM* pTrainer,
             sal_Int32 nNumEpochs = static_cast<sal_Int32>(MAXEPOCHS),
             sal_Int32 nNumIter = static_cast<sal_Int32>(NUMITER));
    ~GMMModel() {}

    double Fit();
    void GetClusterLabels(std::vector<sal_Int32>& rClusterLabels,
                          std::vector<double>& rLabelConfidence, sal_Int32& rNumClusters);

private:
    double dnorm(double fX, double fMean, double fStd);
    double dnormNominal(double fX, double fMean, double fStd);
    void initParms();

    sal_Int32 mnNumClusters;
    const GMM* mpGMM;
    std::vector<std::vector<double>> maWeights;
    std::vector<double> maPhi;
    std::vector<std::vector<double>> maMeans;
    std::vector<std::vector<double>> maStd;
    std::vector<sal_Int32> maClusterLabels;
    std::vector<double> maLabelConfidence;
    double mfBICScore;
    sal_Int32 mnNumEpochs;
    sal_Int32 mnNumIter;
};

class GMM
{
    friend GMMModel;

public:
    GMM(const com::sun::star::uno::Sequence<
            com::sun::star::uno::Sequence<com::sun::star::uno::Any>>& rDataArray,
        const std::vector<DataType>& rColType,
        const std::vector<std::pair<double, double>>& rFeatureScales,
        const sal_Int32 nNumEpochs = static_cast<sal_Int32>(MAXEPOCHS),
        const sal_Int32 nNumIter = static_cast<sal_Int32>(NUMITER));
    ~GMM() {}

    void TrainModel(const std::vector<sal_Int32>& rNumClustersArray);
    void GetClusterLabels(std::vector<sal_Int32>& rClusterLabels,
                          std::vector<double>& rLabelConfidence, sal_Int32& rNumClusters);

private:
    sal_Int32 mnNumSamples;
    sal_Int32 mnNumDimensions;
    std::vector<std::vector<double>> maData;
    std::vector<sal_Int32> maNumClasses;
    std::unique_ptr<GMMModel> mpBestModel;
    sal_Int32 mnNumEpochs;
    sal_Int32 mnNumIter;
};

}