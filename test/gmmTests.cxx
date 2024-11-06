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

#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <em.h>

#include <Eigen/Dense>

#include <fstream>
#include <chrono>
#include <random>

testing::AssertionResult hasCorrectConstLabels(const int* labels, const double* confidences,
                                               int samples, int constLabel, double constConfidence,
                                               const char* contextMsg)
{
    constexpr double precision = 0.01;
    if (!labels)
        return testing::AssertionFailure() << "labels is null";
    if (!confidences)
        return testing::AssertionFailure() << "confidences is null";

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
    int ret = gmmMain(nullptr, rows, cols, 3, 10, 100, labels, confidence, 0);
    EXPECT_EQ(ret, -1);

    ret = gmmMain(&data[0][0], rows, cols, 3, 10, 100, nullptr, confidence, 0);
    EXPECT_EQ(ret, -1);

    ret = gmmMain(&data[0][0], rows, cols, 3, 10, 100, labels, nullptr, 0);
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
    int ret = gmmMain(&dataLow[0][0], rowsLow, cols, 1, 10, 100, labelsLow, confidenceLow, 0);
    EXPECT_EQ(ret, 0);
    // All labels must be 0 with 100% confidence for numClusters = 1 even if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, 0, 1.0,
                                      "[numClusters = 1, #rows < 10]"));

    ret = gmmMain(&dataHigh[0][0], rowsHigh, cols, 1, 10, 100, labelsHigh, confidenceHigh, 0);
    EXPECT_EQ(ret, 0);
    // All labels must be 0 with 100% confidence for numClusters = 1.
    EXPECT_TRUE(hasCorrectConstLabels(labelsHigh, confidenceHigh, rowsHigh, 0, 1.0,
                                      "[numClusters = 1, #rows >= 10]"));

    ret = gmmMain(&dataLow[0][0], rowsLow, cols, 2, 10, 100, labelsLow, confidenceLow, 0);
    EXPECT_EQ(ret, 0);
    // All labels must be -1 with 0% confidence for numSamples != 1 if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, -1, 0.0,
                                      "[numClusters = 2, #rows < 10]"));

    ret = gmmMain(&dataLow[0][0], rowsLow, cols, 0, 10, 100, labelsLow, confidenceLow, 0);
    EXPECT_EQ(ret, 0);
    // All labels must be -1 with 0% confidence for numSamples != 1 if number of samples is < 10.
    EXPECT_TRUE(hasCorrectConstLabels(labelsLow, confidenceLow, rowsLow, -1, 0.0,
                                      "[numClusters = 0, #rows < 10]"));
}

TEST(GMMTests, ThreeClusterCaseDiagonal)
{
    constexpr int numClusters = 3;
    constexpr int rows1Cluster = 300;
    constexpr int rows = rows1Cluster * numClusters;
    constexpr int cols = 5;

    double data[rows][cols];
    std::array<int, rows> rowMap;
    std::array<int, rows> labels;
    for (int row = 0; row < rows; ++row)
        rowMap[row] = row;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::shuffle(rowMap.begin(), rowMap.end(), generator);

    for (int row = 0; row < rows; ++row)
        labels[rowMap[row]] = row / rows1Cluster;

    // std::cerr << "[TRUTH] labels = " << labels[0] << ',' << labels[1] << ',' << labels[2] << '\n';

    std::array<std::array<double, cols>, numClusters> means{
        { { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 3.0, 4.0, 5.0, 1.0, 2.0 }, { 5.0, 1.0, 2.0, 3.0, 4.0 } }
    };
    std::array<std::array<double, cols>, numClusters> stds{ {
        { 2.5, 1.5, 2.5, 1.5, 1.5 },
        { 0.2, 1.5, 1.2, 0.5, 1.2 },
        { 0.7, 2.1, 1.5, 0.7, 1.5 },
    } };

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

    std::array<int, rows> gmmLabels{};
    std::array<double, rows> gmmConfidences{};

    int ret = gmmMain(&data[0][0], rows, cols, numClusters, 10, 50, gmmLabels.data(),
                      gmmConfidences.data(), 0);
    EXPECT_EQ(ret, 0);

    int confusion[numClusters][numClusters]{ { 0 } };

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
        // std::cout << " | " << max << std::endl;
        accuracy += (static_cast<double>(max) / rows1Cluster);
    }

    accuracy /= numClusters;
    EXPECT_GT(accuracy, 0.93);
    EXPECT_LE(accuracy, 1.0);
}

TEST(GMMTests, ThreeClusterCaseFull)
{
    constexpr int numClusters = 3;
    constexpr int rows1Cluster = 300;
    constexpr int rows = rows1Cluster * numClusters;
    constexpr int cols = 2;

    double data[rows][cols];
    std::array<int, rows> rowMap;
    std::array<int, rows> labels;
    for (int row = 0; row < rows; ++row)
        rowMap[row] = row;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::shuffle(rowMap.begin(), rowMap.end(), generator);

    for (int row = 0; row < rows; ++row)
        labels[rowMap[row]] = row / rows1Cluster;

    // std::cerr << "[TRUTH] labels = " << labels[0] << ',' << labels[1] << ',' << labels[2] << '\n';

    std::array<Eigen::Matrix<double, cols, 1>, numClusters> means{
        (Eigen::Matrix<double, cols, 1>() << 0.5, 0.5).finished(),
        (Eigen::Matrix<double, cols, 1>() << 0.0, 1.5).finished(),
        (Eigen::Matrix<double, cols, 1>() << 1.5, 0.0).finished()
    };

    Eigen::Matrix<double, cols, cols> covar{
        (Eigen::Matrix<double, cols, cols>() << 1.0, 0.95, 0.95, 1).finished()
    };

    Eigen::Matrix<double, cols, cols> L(covar.llt().matrixL());

    Eigen::Matrix<double, cols, 1> u;
    Eigen::Matrix<double, cols, 1> output_sample;

    for (int lrow = 0; lrow < rows; ++lrow)
    {
        int row = rowMap[lrow];
        int label = labels[row];

        for (int col = 0; col < cols; ++col)
        {
            std::normal_distribution<double> normalSampler(0.0, 1.0);
            u(col, 0) = normalSampler(generator);
        }

        output_sample = means[label] + (L * u);

        for (int col = 0; col < cols; ++col)
        {
            data[row][col] = output_sample(col, 0);
        }
    }

    // if (true)
    // {
    //     std::ofstream fout("/home/dennis/full.csv");
    //     for (int row = 0; row < rows; ++row)
    //     {
    //         int label = labels[row];
    //         for (int col = 0; col < cols; ++col)
    //         {
    //             fout << data[row][col] << ',';
    //         }
    //         fout << label << std::endl;
    //     }

    //     fout.close();
    // }

    std::array<int, rows> gmmLabels{};
    std::array<double, rows> gmmConfidences{};

    int ret = gmmMain(&data[0][0], rows, cols, numClusters, 10, 50, gmmLabels.data(),
                      gmmConfidences.data(), 1);
    EXPECT_EQ(ret, 0);

    int confusion[numClusters][numClusters]{ { 0 } };

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
        // std::cout << " | " << max << std::endl;
        accuracy += (static_cast<double>(max) / rows1Cluster);
    }

    accuracy /= numClusters;
    EXPECT_GT(accuracy, 0.93);
    EXPECT_LE(accuracy, 1.0);
}
