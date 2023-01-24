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

#include <gtest/gtest.h>
#include "../src/inc/em.h"

#include <chrono>
#include <random>

testing::AssertionResult hasCorrectConstLabels(const int* labels, const double* confidences,
                                               int samples, int constLabel, double constConfidence,
                                               const char* contextMsg)
{
    constexpr double precision = 0.01;
    if (!labels)
        return testing::AssertionFailure() << "labels in null";
    if (!confidences)
        return testing::AssertionFailure() << "confidences in null";

    for (int idx = 0; idx < samples; ++idx)
    {
        if (constLabel != labels[idx])
            return testing::AssertionFailure()
                   << contextMsg << " cluster label for row index " << idx << " is " << labels[idx]
                   << " expected " << constLabel;
        if (std::abs(constConfidence - confidences[idx]) > precision)
            return testing::AssertionFailure()
                   << contextMsg << " cluster label confidence for row index " << idx << " is "
                   << confidences[idx] << " expected to be in " << constConfidence << " +/- "
                   << precision;
    }

    return testing::AssertionSuccess();
}

TEST(GMMTests, ReturnErrorCases)
{
    constexpr int rows = 5;
    constexpr int cols = 3;
    double data[rows][cols];
    int labels[rows];
    double confidence[rows];
    int ret = gmm(nullptr, rows, cols, 3, 10, 100, labels, confidence);
    EXPECT_EQ(ret, -1);

    ret = gmm(&data[0][0], rows, cols, 3, 10, 100, nullptr, confidence);
    EXPECT_EQ(ret, -1);
}

TEST(GMMTests, ConstLabelCases)
{
    constexpr int rowsLow = 5;
    constexpr int rowsHigh = 20;
    constexpr int cols = 3;
    double dataLow[rowsLow][cols]{};
    double dataHigh[rowsHigh][cols]{};
    int labelsLow[rowsLow];
    int labelsHigh[rowsHigh];
    double confidenceLow[rowsLow];
    double confidenceHigh[rowsHigh];
    int ret = gmm(&dataLow[0][0], rowsLow, cols, 1, 10, 100, labelsLow, confidenceLow);
    EXPECT_EQ(ret, 0);
    // All labels must be 0 with 100% confidence for numClusters = 1 even if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, 0, 1.0,
                                      "[numClusters = 1, #rows < 10]"));

    ret = gmm(&dataHigh[0][0], rowsHigh, cols, 1, 10, 100, labelsHigh, confidenceHigh);
    EXPECT_EQ(ret, 0);
    // All labels must be 0 with 100% confidence for numClusters = 1.
    EXPECT_TRUE(hasCorrectConstLabels(labelsHigh, confidenceHigh, rowsHigh, 0, 1.0,
                                      "[numClusters = 1, #rows >= 10]"));

    ret = gmm(&dataLow[0][0], rowsLow, cols, 2, 10, 100, labelsLow, confidenceLow);
    EXPECT_EQ(ret, 0);
    // All labels must be -1 with 0% confidence for numSamples != 1 if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, -1, 0.0,
                                      "[numClusters = 2, #rows < 10]"));

    ret = gmm(&dataLow[0][0], rowsLow, cols, 0, 10, 100, labelsLow, confidenceLow);
    EXPECT_EQ(ret, 0);
    // All labels must be -1 with 0% confidence for numSamples != 1 if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, -1, 0.0,
                                      "[numClusters = 0, #rows < 10]"));
}

TEST(GMMTests, ThreeClusterCase)
{
    constexpr int numClusters = 3;
    constexpr int rows1Cluster = 300;
    constexpr int rows = rows1Cluster * numClusters;
    constexpr int cols = 5;

    double data[rows][cols];
    int rowMap[rows];
    int labels[rows];
    for (int row = 0; row < rows; ++row)
        rowMap[row] = row;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::shuffle(rowMap, rowMap + rows, generator);

    for (int row = 0; row < rows; ++row)
        labels[rowMap[row]] = row / rows1Cluster;

    double means[numClusters][cols]
        = { { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 3.0, 4.0, 5.0, 1.0, 2.0 }, { 5.0, 1.0, 2.0, 3.0, 4.0 } };
    double stds[numClusters][cols] = {
        { 2.5, 1.5, 2.5, 1.5, 1.5 },
        { 0.2, 1.5, 1.2, 0.5, 1.2 },
        { 0.7, 2.1, 1.5, 0.7, 1.5 },
    };

    for (int lrow = 0; lrow < rows; ++lrow)
    {
        int row = rowMap[lrow];
        int label = labels[row];

        for (int col = 0; col < cols; ++col)
        {
            std::normal_distribution<double> normalSampler(means[label][col], stds[label][col]);
            data[row][col] = normalSampler(generator);
        }
    }

    int gmmLabels[rows];
    double gmmConfidences[rows];

    int ret = gmm(&data[0][0], rows, cols, numClusters, 40, 100, gmmLabels, gmmConfidences);
    EXPECT_EQ(ret, 0);

    int confusion[numClusters][numClusters]{ 0 };

    for (int row = 0; row < rows; ++row)
    {
        int actualLabel = gmmLabels[row];
        ASSERT_LT(actualLabel, numClusters) << " for row " << row;
        ASSERT_GE(actualLabel, 0) << " for row " << row;
        ++confusion[labels[row]][gmmLabels[row]];
    }

    double accuracy = 0;
    int realToActual[numClusters];
    for (int real = 0; real < numClusters; ++real)
    {
        int max = 0;
        for (int actual = 0; actual < numClusters; ++actual)
        {
            int count = confusion[real][actual];
            //std::cout << count << "  ";
            if (count > max)
            {
                max = count;
                realToActual[real] = actual;
            }
        }

        for (int prevReal = 0; prevReal < real; ++prevReal)
        {
            EXPECT_NE(realToActual[real], realToActual[prevReal])
                << " both the real labels " << real << " and " << prevReal
                << " are mapped to the same actual label " << realToActual[real] << " !";
        }
        //std::cout << " | " << max << std::endl;
        accuracy += (static_cast<double>(max) / rows1Cluster);
    }

    accuracy /= numClusters;
    EXPECT_GT(accuracy, 0.93);
    EXPECT_LE(accuracy, 1.0);
}
