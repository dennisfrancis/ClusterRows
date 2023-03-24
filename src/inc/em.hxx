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

#include "em.h"
#include "datamatrix.hxx"
#include <memory>
#include <vector>

#define EPSILON 0.001

namespace em
{
class GMM;

class GMMModel
{
public:
    /// @brief Stores a gaussian mixture model
    /// @param numClusters desired number of clusters (auto determines it if 0 is provided)
    /// @param rTrainer GMM trainer object
    /// @param numEpochs desired number of epochs
    /// @param numIter desired number of iteration in each epoch
    GMMModel(int numClusters, const GMM& rTrainer, int numEpochs, int numIter);
    ~GMMModel() = default;

    double Fit();
    void GetClusterLabels(int* clusterLabels, double* labelConfidence);

private:
    static double dnorm(double fX, double fMean, double fStd);
    void initParms();
    double runEpoch(int epochIndex, std::vector<double>& epochLabelConfidence,
                    std::vector<int>& epochClusterLabels, std::vector<double>& tmpLabelConfidence,
                    std::vector<int>& tmpClusterLabels);

    int m_numClusters;
    const GMM& m_rGMM;
    std::vector<std::vector<double>> m_weights;
    std::vector<double> m_phi;
    std::vector<std::vector<double>> m_means;
    std::vector<std::vector<double>> m_std;
    std::vector<int> m_clusterLabels;
    std::vector<double> m_labelConfidence;
    double m_BICScore;
    int m_numEpochs;
    int m_numIter;
};

class GMM
{
    friend GMMModel;

public:
    GMM(const double* pRows, int nRows, int nCols, int nNumEpochs, int nNumIter);
    ~GMM() = default;

    void TrainModel(const std::vector<int>& numClustersArray);
    void GetClusterLabels(int* clusterLabels, double* labelConfidence);

private:
    void computeStats();
    [[nodiscard]]
    double getNormalized(int row, int col) const;

private:
    int mnNumSamples;
    int mnNumDimensions;
    util::DataMatrix maData;
    std::unique_ptr<GMMModel> mpBestModel;
    int mnNumEpochs;
    int mnNumIter;
    std::vector<double> maStds;
    std::vector<double> maMeans;
};

}