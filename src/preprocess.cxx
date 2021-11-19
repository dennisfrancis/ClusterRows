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

#include "preprocess.hxx"
#include "logging.hxx"

#include <rtl/ustring.hxx>

#include <unordered_set>
#include <math.h>
#include <cassert>
#include <algorithm>

#define EMPTYSTRING OUString("__NA__")
#define EMPTYDOUBLE -9999999.0

using com::sun::star::uno::Any;
using com::sun::star::uno::Sequence;
using rtl::OUString;
using rtl::OUStringHash;

void preprocess::getColTypes(const Sequence<Sequence<Any>>& rData, std::vector<DataType>& rColType,
                             std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx)
{
    sal_Int32 nNumRows = rData.getLength();
    assert(nNumRows && "nNumRows cannot be 0!");

    sal_Int32 nNumCols = rData[0].getLength();

    for (sal_Int32 nCol = 0; nCol < nNumCols; ++nCol)
    {
        DataType aType = DataType::INTEGER;
        bool bIsComplete = true;
        std::vector<sal_Int32> aBlankRowIdx;
        double fMin = 1.0E10, fMax = -1.0E10;

        for (sal_Int32 nRow = 0; nRow < nNumRows; ++nRow)
        {
            Any aVal = rData[nRow][nCol];
            OUString aTest;
            double fVal;
            if (!aVal.hasValue())
            {
                bIsComplete = false;
                aBlankRowIdx.push_back(nRow);
                continue;
            }

            if (aType != DataType::STRING && (aVal >>= aTest))
            {
                aType = DataType::STRING;
                if (!bIsComplete)
                    break;
            }

            else if (aType != DataType::STRING && (aVal >>= fVal))
            {
                if (fVal != static_cast<double>(static_cast<sal_Int64>(fVal)))
                    aType = DataType::DOUBLE;
                if (aType == DataType::INTEGER)
                {
                    fMin = (fMin > fVal) ? fVal : fMin;
                    fMax = (fMax < fVal) ? fVal : fMax;
                }
            }
        }

        if (aType == DataType::INTEGER && (fMax - fMin) > 100.0)
            aType = DataType::DOUBLE;

        rColType[nCol] = aType;
        rCol2BlankRowIdx[nCol] = std::move(aBlankRowIdx);

        writeLog("DEBUG>>> col = %d, Type = %s, isComplete = %d\n", nCol, DataType2String(aType),
                 int(bIsComplete));
    }
}

void preprocess::flagEmptyEntries(Sequence<Sequence<Any>>& rDataArray,
                                  const std::vector<DataType>& rColType,
                                  const std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx)
{
    sal_Int32 nNumCols = rColType.size();
    for (sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
    {
        for (sal_Int32 nRowIdx : rCol2BlankRowIdx[nColIdx])
        {
            if (rColType[nColIdx] == DataType::STRING)
                rDataArray[nRowIdx][nColIdx] <<= EMPTYSTRING;
            else
                rDataArray[nRowIdx][nColIdx] <<= EMPTYDOUBLE;
        }
    }
}

void preprocess::imputeAllColumns(Sequence<Sequence<Any>>& rDataArray,
                                  std::vector<DataType>& rColType,
                                  const std::vector<std::vector<sal_Int32>>& rCol2BlankRowIdx)
{
    sal_Int32 nNumCols = rColType.size();
    for (sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
    {
        if (rColType[nColIdx] == DataType::STRING)
            preprocess::imputeWithMode(rDataArray, nColIdx, rColType[nColIdx],
                                       rCol2BlankRowIdx[nColIdx]);
        else if (rColType[nColIdx] == DataType::DOUBLE)
            preprocess::imputeWithMedian(rDataArray, nColIdx, rColType[nColIdx],
                                         rCol2BlankRowIdx[nColIdx]);
        else if (rColType[nColIdx] == DataType::INTEGER)
        {
            if (!preprocess::imputeWithMode(rDataArray, nColIdx, rColType[nColIdx],
                                            rCol2BlankRowIdx[nColIdx]))
            {
                // Better to treat the numbers as continuous rather than discrete classes.
                //rColType[nColIdx] = DataType::DOUBLE;
                preprocess::imputeWithMedian(rDataArray, nColIdx, rColType[nColIdx],
                                             rCol2BlankRowIdx[nColIdx]);
            }
        }
    }
}

bool preprocess::imputeWithMode(Sequence<Sequence<Any>>& rDataArray, const sal_Int32 nColIdx,
                                const DataType aType,
                                const std::vector<sal_Int32>& rEmptyRowIndices)
{
    std::unordered_multiset<OUString, OUStringHash> aStringMultiSet;
    std::unordered_multiset<double> aDoubleMultiSet;
    OUString aImputeString;
    double fImputeDouble;
    sal_Int32 nMaxCount = 0;
    sal_Int32 nNumRows = rDataArray.getLength();
    for (sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx)
    {
        Any aElement = rDataArray[nRowIdx][nColIdx];
        if ((aType == DataType::STRING && aElement == EMPTYSTRING)
            || (aType == DataType::DOUBLE && aElement == EMPTYDOUBLE))
            continue;

        sal_Int32 nCount = 0;
        if (aType == DataType::STRING)
        {
            OUString aStr;
            aElement >>= aStr;
            aStringMultiSet.insert(aStr);
            nCount = aStringMultiSet.count(aStr);
        }
        else
        {
            double fVal;
            aElement >>= fVal;
            aDoubleMultiSet.insert(fVal);
            nCount = aDoubleMultiSet.count(fVal);
        }
        if (nCount > nMaxCount)
        {
            if (aType == DataType::STRING)
                aElement >>= aImputeString;
            else
                aElement >>= fImputeDouble;

            nMaxCount = nCount;
        }
    }

    bool bGood = true;
    if (aType == DataType::INTEGER)
    {
        if (nMaxCount < 3) // Ensure at least 3 samples of top class
            bGood = false;
    }

    if (bGood)
    {
        if (aType == DataType::STRING)
            for (sal_Int32 nMissingIdx : rEmptyRowIndices)
                rDataArray[nMissingIdx][nColIdx] <<= aImputeString;
        else
            for (sal_Int32 nMissingIdx : rEmptyRowIndices)
                rDataArray[nMissingIdx][nColIdx] <<= fImputeDouble;
    }

    return bGood;
}

bool preprocess::imputeWithMedian(Sequence<Sequence<Any>>& rDataArray, const sal_Int32 nColIdx,
                                  const DataType aType,
                                  const std::vector<sal_Int32>& rEmptyRowIndices)
{
    // We are sure that this function is not called for Any == OUString
    assert(aType != DataType::STRING && "imputeWithMedian called with type OUString !!!");

    sal_Int32 nNumRows = rDataArray.getLength();
    sal_Int32 nNumEmptyElements = rEmptyRowIndices.size();
    std::vector<double> aCopy(nNumRows);
    for (sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx)
        rDataArray[nRowIdx][nColIdx] >>= aCopy[nRowIdx];

    std::sort(aCopy.begin(), aCopy.end());
    size_t nElements = nNumRows - nNumEmptyElements;
    double fMedian;

    if ((nElements % 2) == 0)
    {
        double fMed1 = aCopy[nNumEmptyElements + (nElements / 2)];
        double fMed2 = aCopy[nNumEmptyElements + (nElements / 2) - 1];
        fMedian = 0.5 * (fMed1 + fMed2);
    }
    else
        fMedian = aCopy[nNumEmptyElements + (nElements / 2)];

    for (sal_Int32 nMissingIdx : rEmptyRowIndices)
        rDataArray[nMissingIdx][nColIdx] <<= fMedian;

    return true;
}

void preprocess::calculateFeatureScales(Sequence<Sequence<Any>>& rDataArray,
                                        const std::vector<DataType>& rColType,
                                        std::vector<std::pair<double, double>>& rFeatureScales)
{
    sal_Int32 nNumRows = rDataArray.getLength();
    sal_Int32 nNumCols = rColType.size();

    for (sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
    {
        if (rColType[nColIdx] == DataType::STRING)
            continue;
        double fSum = 0.0, fSum2 = 0.0;
        for (sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx)
        {
            double fVal;
            rDataArray[nRowIdx][nColIdx] >>= fVal;
            fSum += fVal;
            fSum2 += (fVal * fVal);
        }
        double fMean = fSum / static_cast<double>(nNumRows);
        double fStd = sqrt((fSum2 / static_cast<double>(nNumRows)) - (fMean * fMean));
        rFeatureScales[nColIdx].first = fMean;
        // Avoid 0 standard deviation condition.
        rFeatureScales[nColIdx].second = (fStd == 0.0) ? fMean : fStd;
    }
}
