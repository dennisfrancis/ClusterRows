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
#include <memory>
#include <vector>

#define EPSILON 0.001

namespace em
{
class DataMatrix
{
public:
    /// @brief Accepts a externally owned double array to use it as a column major matrix.
    /// @param pRows externally owned array
    /// @param nRows number of rows
    /// @param nCols number of columns
    DataMatrix(const double* pRows, const int nRows, const int nCols)
        : pData(pRows)
        , mnRows(nRows)
        , mnCols(nCols)
    {
    }

    ~DataMatrix() {}

    const double* operator[](int nRow) const { return pData + (nRow * mnCols); }

private:
    const double* pData;
    const int mnRows;
    const int mnCols;
};

class GMM;

class GMMModel
{
public:
    /// @brief Stores a gaussian mixture model
    /// @param numClusters desired number of clusters (auto determines it if 0 is provided)
    /// @param rTrainer GMM trainer object
    /// @param numEpochs desired number of epochs
    /// @param numIter desired number of iteration in each epoch
    GMMModel(const int numClusters, const GMM& rTrainer, int numEpochs, int numIter);
    ~GMMModel() {}

    double Fit();
    void GetClusterLabels(int* clusterLabels, double* labelConfidence);

private:
    double dnorm(double fX, double fMean, double fStd);
    void initParms();

    int mnNumClusters;
    const GMM& mrGMM;
    std::vector<std::vector<double>> maWeights;
    std::vector<double> maPhi;
    std::vector<std::vector<double>> maMeans;
    std::vector<std::vector<double>> maStd;
    std::vector<int> maClusterLabels;
    std::vector<double> maLabelConfidence;
    double mfBICScore;
    int mnNumEpochs;
    int mnNumIter;
};

class GMM
{
    friend GMMModel;

public:
    GMM(const double* pRows, int nRows, int nCols, int nNumEpochs, int nNumIter);
    ~GMM() {}

    void TrainModel(const std::vector<int>& numClustersArray);
    void GetClusterLabels(int* clusterLabels, double* labelConfidence);

private:
    void computeStats();
    double getNormalized(int row, int col) const;

private:
    int mnNumSamples;
    int mnNumDimensions;
    DataMatrix maData;
    std::unique_ptr<GMMModel> mpBestModel;
    int mnNumEpochs;
    int mnNumIter;
    std::vector<double> maStds;
    std::vector<double> maMeans;
};

}