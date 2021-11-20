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

#include "colorgen.hxx"
#include <cmath>

void getClusterColors(const sal_Int32 nNumClusters, std::vector<sal_Int32>& rClusterColors)
{
    double fHueSlice = 360.0 / nNumClusters;
    constexpr double fL = 0.5;
    constexpr double fS = 0.75;
    for (sal_Int32 nClusterIdx = 0; nClusterIdx < nNumClusters; ++nClusterIdx)
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
