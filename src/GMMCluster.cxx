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

#include "GMMCluster.hxx"
#include "helper.hxx"
#include "logging.hxx"
#include "perf.hxx"
#include "datatypes.hxx"
#include "preprocess.hxx"
#include "em.hxx"

#include <vector>

using rtl::OUString;
using namespace com::sun::star::uno;

using namespace helper;
using namespace preprocess;
using namespace em;

// This is the service name an Add-On has to implement
#define ADDIN_BASE_SERVICE_NAME "com.sun.star.sheet.AddIn"
#define ADDIN_SERVICE_NAME "com.github.dennisfrancis.GMMCluster"

// Helper functions for the implementation of UNO component interfaces.
OUString GMMClusterImpl_getImplementationName() { return OUString(ADDIN_IMPLEMENTATION_NAME); }

Sequence<OUString> SAL_CALL GMMClusterImpl_getSupportedServiceNames()
{
    Sequence<OUString> aRet(2);
    OUString* pArray = aRet.getArray();
    pArray[0] = OUString(ADDIN_BASE_SERVICE_NAME);
    pArray[1] = OUString(ADDIN_SERVICE_NAME);
    return aRet;
}

Reference<XInterface>
    SAL_CALL GMMClusterImpl_createInstance(const Reference<XComponentContext>& /*rContext*/)
{
    return (cppu::OWeakObject*)new GMMClusterImpl();
}

OUString GMMClusterImpl::getServiceName() { return ADDIN_SERVICE_NAME; }

// Implementation of the recommended/mandatory interfaces of a UNO component.
// XServiceInfo
OUString SAL_CALL GMMClusterImpl::getImplementationName()
{
    return GMMClusterImpl_getImplementationName();
}

sal_Bool SAL_CALL GMMClusterImpl::supportsService(const OUString& rServiceName)
{
    return (rServiceName == ADDIN_BASE_SERVICE_NAME || rServiceName == ADDIN_SERVICE_NAME);
}

Sequence<OUString> SAL_CALL GMMClusterImpl::getSupportedServiceNames()
{
    return GMMClusterImpl_getSupportedServiceNames();
}

const OUString GMMClusterImpl::aFunctionNames[NUMFUNCTIONS] = {
    "gmmCluster",
};

const OUString GMMClusterImpl::aDisplayFunctionNames[NUMFUNCTIONS] = {
    "gmmcluster",
};

const OUString GMMClusterImpl::aDescriptions[NUMFUNCTIONS] = {
    "Computes cluster index and membership confidence scores",
};

const OUString GMMClusterImpl::aArgumentNames[NUMFUNCTIONS][NUMARGS] = {
    {
        "data",
        "numClusters",
        "numEpochs",
        "numIterations",
    },
};

const OUString GMMClusterImpl::aArgumentDescriptions[NUMFUNCTIONS][NUMARGS] = {
    {
        "cell range of data to be clustered",
        "number of clusters (optional)",
        "number of epochs (optional)",
        "number of iterations (optional)",
    },
};

sal_Int32 GMMClusterImpl::getFunctionID(const OUString aProgrammaticFunctionName) const
{
    writeLog("getFunctionID : aProgrammat = %s\n", aProgrammaticFunctionName.toUtf8().getStr());
    fflush(stdout);
    for (sal_Int32 nIdx = 0; nIdx < nNumFunctions; ++nIdx)
        if (aProgrammaticFunctionName == aFunctionNames[nIdx])
            return nIdx;
    return -1;
}

namespace
{
void getParamNumber(const Any& param, sal_Int32& nParam, const char* name)
{
    if (!param.hasValue())
    {
        writeLog("param %s has no value!", name);
        return;
    }
    double fVal = 0;
    if (param >>= fVal)
        nParam = static_cast<sal_Int32>(fVal);
}

}

Sequence<Sequence<double>>
    SAL_CALL GMMClusterImpl::gmmCluster(const Sequence<Sequence<Any>>& dataConst,
                                        const Any& numClusters, const Any& numEpochs,
                                        const Any& numIterations)
{
    if (!dataConst.getLength())
        return Sequence<Sequence<double>>();

    const sal_Int32 nNumRows = dataConst.getLength();
    const sal_Int32 nNumCols = dataConst[0].getLength();
    Sequence<Sequence<Any>> data(dataConst.getLength());
    for (sal_Int32 nIdx = 0; nIdx < nNumRows; ++nIdx)
        data[nIdx] = dataConst[nIdx];

    TimePerf aPerfPreprocess("dataPreprocess");

    std::vector<DataType> aColType(nNumCols);
    std::vector<std::vector<sal_Int32>> aCol2BlankRowIdx(nNumCols);

    getColTypes(data, aColType, aCol2BlankRowIdx);
    flagEmptyEntries(data, aColType, aCol2BlankRowIdx);
    imputeAllColumns(data, aColType, aCol2BlankRowIdx);
    std::vector<std::pair<double, double>> aFeatureScales(nNumCols);
    calculateFeatureScales(data, aColType, aFeatureScales);
    for (sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
    {
        writeLog("DEBUG>>> col %d has type %d, mean = %.4f, std = %.5f\n", nColIdx,
                 aColType[nColIdx], aFeatureScales[nColIdx].first, aFeatureScales[nColIdx].second);
    }
    aPerfPreprocess.Stop();

    TimePerf aPerfCompute("computeClusters");
    std::vector<sal_Int32> aClusterLabels(nNumRows);
    std::vector<double> aLabelConfidence(nNumRows);
    sal_Int32 nNumClusters = 0;
    sal_Int32 nNumEpochs = static_cast<sal_Int32>(MAXEPOCHS);
    sal_Int32 nNumIter = static_cast<sal_Int32>(NUMITER);
    getParamNumber(numClusters, nNumClusters, "numClusters");
    getParamNumber(numEpochs, nNumEpochs, "numEpochs");
    getParamNumber(numIterations, nNumIter, "numIterations");
    writeLog("PARAMETER values: numClusters = %d, numEpoch = %d, numIterations = %d\n",
             nNumClusters, nNumEpochs, nNumIter);
    performEMClustering(data, aColType, aFeatureScales, aClusterLabels, aLabelConfidence,
                        nNumClusters, nNumEpochs, nNumIter);
    Sequence<Sequence<double>> aSeq(nNumRows);
    for (sal_Int32 nRow = 0; nRow < nNumRows; ++nRow)
    {
        aSeq[nRow].realloc(2);
        aSeq[nRow][0] = static_cast<double>(aClusterLabels[nRow]),
        aSeq[nRow][1] = aLabelConfidence[nRow];
    }

    return aSeq;
}

OUString GMMClusterImpl::getProgrammaticFuntionName(const OUString& aDisplayName)
{
    writeLog("getProgrammaticFuntionName : aDisplayName = %s\n", aDisplayName.toUtf8().getStr());
    fflush(stdout);
    for (sal_Int32 nIdx = 0; nIdx < nNumFunctions; ++nIdx)
        if (aDisplayName == aDisplayFunctionNames[nIdx])
            return aFunctionNames[nIdx];
    return "";
}

OUString GMMClusterImpl::getDisplayFunctionName(const OUString& aProgrammaticName)
{
    writeLog("getProgrammaticFuntionName : aPro = %s\n", aProgrammaticName.toUtf8().getStr());
    fflush(stdout);
    sal_Int32 nFIdx = getFunctionID(aProgrammaticName);
    return (nFIdx == -1) ? "" : aDisplayFunctionNames[nFIdx];
}

OUString GMMClusterImpl::getFunctionDescription(const OUString& aProgrammaticName)
{
    sal_Int32 nFIdx = getFunctionID(aProgrammaticName);
    return (nFIdx == -1) ? "" : aDescriptions[nFIdx];
}

OUString GMMClusterImpl::getDisplayArgumentName(const OUString& aProgrammaticFunctionName,
                                                sal_Int32 nArgument)
{
    sal_Int32 nFIdx = getFunctionID(aProgrammaticFunctionName);
    return (nFIdx == -1 || (nArgument < 0 || nArgument >= NUMARGS))
               ? ""
               : aArgumentNames[nFIdx][nArgument];
}

OUString GMMClusterImpl::getArgumentDescription(const OUString& aProgrammaticFunctionName,
                                                sal_Int32 nArgument)
{
    sal_Int32 nFIdx = getFunctionID(aProgrammaticFunctionName);
    return (nFIdx == -1 || (nArgument < 0 || nArgument >= NUMARGS))
               ? ""
               : aArgumentDescriptions[nFIdx][nArgument];
}

OUString GMMClusterImpl::getProgrammaticCategoryName(const OUString& /*aProgrammaticFunctionName*/)
{
    return "Add-In";
}

OUString GMMClusterImpl::getDisplayCategoryName(const OUString& /*aProgrammaticFunctionName*/)
{
    return "Add-In";
}
