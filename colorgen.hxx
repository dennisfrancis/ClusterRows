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
#include <vector>
#include <cmath>

#include "logging.hxx"
#include "datatypes.hxx"

void getRowColors(const std::vector<sal_Int32>& rClusterLabels,
                  const std::vector<double>& rLabelConfidence, const sal_Int32 nNumClusters,
                  std::vector<sal_Int32>& rRowColors);

void getLabelBaseColors(const sal_Int32 nNumClusters, std::vector<ColorsRGB>& rBaseColors);
void getClusterColors(const sal_Int32 nNumClusters, std::vector<sal_Int32>& rClusterColors);

void getClusterColors(const sal_Int32 nNumClusters, std::vector<sal_Int32>& rClusterColors)
{
    double fHueSlice = 360.0 / nNumClusters;
    constexpr double fL = 0.5;
    constexpr double fS = 1;
    for (sal_Int32 nClusterIdx; nClusterIdx < nNumClusters; ++nClusterIdx)
    {
        // https://en.wikipedia.org/wiki/HSL_and_HSV
        // HSL to RGB conversion formula.

        const double fH = fHueSlice * nClusterIdx; // Cluster hue.
        const double fC = (1 - std::abs(2 * fL - 1)) * fS; // Cluster chroma.
        const double fHP = fH / 60.0;
        const double fX = fC * (1 - std::abs(std::fmod(fHP, 2) - 1));
        const int nHP = fHP;
        double fR, fG, fB;

        switch (nHP)
        {
            case 0:
                fR = fC;
                fG = fX;
                fB = 0;
                break;
            case 1:
                fR = fX;
                fG = fC;
                fB = 0;
                break;
            case 2:
                fR = 0;
                fG = fC;
                fB = fX;
                break;
            case 3:
                fR = 0;
                fG = fX;
                fB = fC;
                break;
            case 4:
                fR = fX;
                fG = 0;
                fB = fC;
                break;
            case 5:
                fR = fC;
                fG = 0;
                fB = fX;
                break;
            default:
                fR = 0;
                fG = 0;
                fB = 0;
        }

        double fM = fL - fC / 2;
        fR += fM;
        fG += fM;
        fB += fM;

        rClusterColors[nClusterIdx] = (static_cast<sal_Int32>(255 * fR) << 16)
                                      | (static_cast<sal_Int32>(255 * fG) << 8)
                                      | static_cast<sal_Int32>(255 * fB);
    }
}

void getLabelBaseColors(const sal_Int32 nNumClusters, std::vector<ColorsRGB>& rBaseColors)
{
    rBaseColors[0] = { 1.0, 0.11, 0.11 };
    rBaseColors[1] = { 0.11, 1.0, 0.11 };
    if (nNumClusters > 2)
    {
        rBaseColors[2] = { 0.11, 0.11, 1.0 };
        for (sal_Int32 nClusterIdx = 3; nClusterIdx < nNumClusters; nClusterIdx += 3)
        {
            rBaseColors[nClusterIdx].Red
                = 0.5 * (rBaseColors[nClusterIdx - 3].Red + rBaseColors[nClusterIdx - 2].Red);
            rBaseColors[nClusterIdx].Green
                = 0.5 * (rBaseColors[nClusterIdx - 3].Green + rBaseColors[nClusterIdx - 2].Green);
            rBaseColors[nClusterIdx].Blue
                = 0.5 * (rBaseColors[nClusterIdx - 3].Blue + rBaseColors[nClusterIdx - 2].Blue);

            if ((nClusterIdx + 1) >= nNumClusters)
                break;
            rBaseColors[nClusterIdx + 1].Red
                = 0.5 * (rBaseColors[nClusterIdx - 2].Red + rBaseColors[nClusterIdx - 1].Red);
            rBaseColors[nClusterIdx + 1].Green
                = 0.5 * (rBaseColors[nClusterIdx - 2].Green + rBaseColors[nClusterIdx - 1].Green);
            rBaseColors[nClusterIdx + 1].Blue
                = 0.5 * (rBaseColors[nClusterIdx - 2].Blue + rBaseColors[nClusterIdx - 1].Blue);

            if ((nClusterIdx + 2) >= nNumClusters)
                break;
            rBaseColors[nClusterIdx + 2].Red
                = 0.5 * (rBaseColors[nClusterIdx - 3].Red + rBaseColors[nClusterIdx - 1].Red);
            rBaseColors[nClusterIdx + 2].Green
                = 0.5 * (rBaseColors[nClusterIdx - 3].Green + rBaseColors[nClusterIdx - 1].Green);
            rBaseColors[nClusterIdx + 2].Blue
                = 0.5 * (rBaseColors[nClusterIdx - 3].Blue + rBaseColors[nClusterIdx - 1].Blue);
        }
    }
}

void getRowColors(const std::vector<sal_Int32>& rClusterLabels,
                  const std::vector<double>& rLabelConfidence, const sal_Int32 nNumClusters,
                  std::vector<sal_Int32>& rRowColors)
{
    for (auto nClusterIdx : rClusterLabels)
        writeLog("%d, ", nClusterIdx);
    writeLog("\n");
    std::vector<ColorsRGB> aBaseColors(nNumClusters);
    getLabelBaseColors(nNumClusters, aBaseColors);

    sal_Int32 nNumRows = rClusterLabels.size();
    for (sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx)
    {
        sal_Int32 nClusterIdx = rClusterLabels[nRowIdx];
        double fConfidence = rLabelConfidence[nRowIdx];
        // Invert fConfidence and bring to the scale [0.5, 1.0]
        double fColorScale = (1 - fConfidence) * 0.5 + 0.5;
        ColorsRGB aBaseColor = aBaseColors[nClusterIdx];
        // High confidence ==> darker  version of the base color
        // Low  confidence ==> lighter version of the base color
        ColorsRGB aRowColor = { aBaseColor.Red * fColorScale, aBaseColor.Green * fColorScale,
                                aBaseColor.Blue * fColorScale };
        const sal_Int32 nColrRed = static_cast<sal_Int32>(255.0 * aRowColor.Red + 0.5);
        const sal_Int32 nColrGreen = static_cast<sal_Int32>(255.0 * aRowColor.Green + 0.5);
        const sal_Int32 nColrBlue = static_cast<sal_Int32>(255.0 * aRowColor.Blue + 0.5);
        rRowColors[nRowIdx] = ((nColrRed << 16) | (nColrGreen << 8) | nColrBlue);
    }
}
