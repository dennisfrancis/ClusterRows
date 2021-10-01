
#ifndef __CLUSTERROWS_COLORGEN__
#define __CLUSTERROWS_COLORGEN__

#include <cppu/unotype.hxx>
#include <vector>

#include "logging.hxx"
#include "datatypes.hxx"

void getRowColors(const std::vector<sal_Int32> &rClusterLabels,
                  const std::vector<double> &rLabelConfidence,
                  const sal_Int32 nNumClusters,
                  std::vector<sal_Int32> &rRowColors);

void getLabelBaseColors(const sal_Int32 nNumClusters,
                        std::vector<ColorsRGB> &rBaseColors);

void getLabelBaseColors(const sal_Int32 nNumClusters,
                        std::vector<ColorsRGB> &rBaseColors)
{
    rBaseColors[0] = {1.0, 0.11, 0.11};
    rBaseColors[1] = {0.11, 1.0, 0.11};
    if (nNumClusters > 2)
    {
        rBaseColors[2] = {0.11, 0.11, 1.0};
        for (sal_Int32 nClusterIdx = 3; nClusterIdx < nNumClusters; nClusterIdx += 3)
        {
            rBaseColors[nClusterIdx].Red = 0.5 * (rBaseColors[nClusterIdx - 3].Red + rBaseColors[nClusterIdx - 2].Red);
            rBaseColors[nClusterIdx].Green = 0.5 * (rBaseColors[nClusterIdx - 3].Green + rBaseColors[nClusterIdx - 2].Green);
            rBaseColors[nClusterIdx].Blue = 0.5 * (rBaseColors[nClusterIdx - 3].Blue + rBaseColors[nClusterIdx - 2].Blue);

            if ((nClusterIdx + 1) >= nNumClusters)
                break;
            rBaseColors[nClusterIdx + 1].Red = 0.5 * (rBaseColors[nClusterIdx - 2].Red + rBaseColors[nClusterIdx - 1].Red);
            rBaseColors[nClusterIdx + 1].Green = 0.5 * (rBaseColors[nClusterIdx - 2].Green + rBaseColors[nClusterIdx - 1].Green);
            rBaseColors[nClusterIdx + 1].Blue = 0.5 * (rBaseColors[nClusterIdx - 2].Blue + rBaseColors[nClusterIdx - 1].Blue);

            if ((nClusterIdx + 2) >= nNumClusters)
                break;
            rBaseColors[nClusterIdx + 2].Red = 0.5 * (rBaseColors[nClusterIdx - 3].Red + rBaseColors[nClusterIdx - 1].Red);
            rBaseColors[nClusterIdx + 2].Green = 0.5 * (rBaseColors[nClusterIdx - 3].Green + rBaseColors[nClusterIdx - 1].Green);
            rBaseColors[nClusterIdx + 2].Blue = 0.5 * (rBaseColors[nClusterIdx - 3].Blue + rBaseColors[nClusterIdx - 1].Blue);
        }
    }
}

void getRowColors(const std::vector<sal_Int32> &rClusterLabels,
                  const std::vector<double> &rLabelConfidence,
                  const sal_Int32 nNumClusters,
                  std::vector<sal_Int32> &rRowColors)
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
        ColorsRGB aRowColor = {aBaseColor.Red * fColorScale,
                               aBaseColor.Green * fColorScale,
                               aBaseColor.Blue * fColorScale};
        const sal_Int32 nColrRed = static_cast<sal_Int32>(255.0 * aRowColor.Red + 0.5);
        const sal_Int32 nColrGreen = static_cast<sal_Int32>(255.0 * aRowColor.Green + 0.5);
        const sal_Int32 nColrBlue = static_cast<sal_Int32>(255.0 * aRowColor.Blue + 0.5);
        rRowColors[nRowIdx] = ((nColrRed << 16) | (nColrGreen << 8) | nColrBlue);
    }
}

#endif
