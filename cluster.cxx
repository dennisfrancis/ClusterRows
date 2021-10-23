#include "cluster.hxx"
#include "GMMCluster.hxx"
#include "perf.hxx"

#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <com/sun/star/frame/DispatchResultEvent.hpp>
#include <com/sun/star/frame/DispatchResultState.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XModel.hpp>

#include <com/sun/star/uno/XComponentContext.hpp>

#include <com/sun/star/sheet/XSheetCellRanges.hpp>
#include <com/sun/star/sheet/XCellRangeAddressable.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/sheet/XSheetCellCursor.hpp>
#include <com/sun/star/sheet/XCellRangeData.hpp>

#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>

#include <com/sun/star/table/XCellRange.hpp>

#include <com/sun/star/util/Color.hpp>

#include <com/sun/star/document/XUndoManager.hpp>
#include <com/sun/star/document/XUndoManagerSupplier.hpp>

#include <cppuhelper/supportsservice.hxx>

#include <vector>
#include <chrono>
#include <thread>

#include "range.hxx"
#include "preprocess.hxx"
#include "colorgen.hxx"
#include "em.hxx"

using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::sheet;
using namespace com::sun::star::table;
using com::sun::star::beans::NamedValue;
using com::sun::star::beans::XPropertySet;
using com::sun::star::container::XIndexAccess;
using com::sun::star::document::XUndoManager;
using com::sun::star::document::XUndoManagerSupplier;
using com::sun::star::frame::DispatchResultEvent;
using com::sun::star::lang::IllegalArgumentException;
using com::sun::star::util::Color;

// This is the service name an Add-On has to implement
#define SERVICE_NAME "com.sun.star.task.Job"

void logError(const char *pStr);
Reference<XModel> getModel(const Reference<XFrame> &rxFrame);
bool getDataRange(const Reference<XModel> &rxModel, CellRangeAddress &rRangeExtended);
Reference<XSpreadsheet> getSheet(const Reference<XModel> &rxModel, const sal_Int32 nSheet);

bool getClusterLabels(const Sequence<Sequence<Any>> &rDataArray,
                      const std::vector<DataType> &rColType,
                      const std::vector<std::pair<double, double>> &rFeatureScales,
                      std::vector<sal_Int32> &rClusterLabels,
                      std::vector<double> &rLabelConfidence,
                      sal_Int32 &rNumClusters);

sal_Bool clusterColorRows(const Reference<XSpreadsheet> &rxSheet,
                          const CellRangeAddress &rRange,
                          const sal_Int32 nUserNumClusters);

// Helper functions for the implementation of UNO component interfaces.
OUString ClusterRowsImpl_getImplementationName()
{
    return OUString(IMPLEMENTATION_NAME);
}

Sequence<OUString> SAL_CALL ClusterRowsImpl_getSupportedServiceNames()
{
    Sequence<OUString> aRet(1);
    OUString *pArray = aRet.getArray();
    pArray[0] = OUString(SERVICE_NAME);
    return aRet;
}

Reference<XInterface> SAL_CALL ClusterRowsImpl_createInstance(const Reference<XComponentContext> &rContext)
{
    return (cppu::OWeakObject *)new ClusterRowsImpl(rContext);
}

ClusterRowsImpl::ClusterRowsImpl(const Reference<XComponentContext> &rxContext):
    mxContext(rxContext)
{
    writeLog("DEBUG>>> Created ClusterRowsImpl object : %p\n", this);
}

ClusterRowsImpl::~ClusterRowsImpl()
{
    writeLog("DEBUG>>> Destructing ClusterRowsImpl object : %p\n", this);
}

// Implementation of the recommended/mandatory interfaces of a UNO component.
// XServiceInfo
OUString SAL_CALL ClusterRowsImpl::getImplementationName()
{
    return ClusterRowsImpl_getImplementationName();
}

sal_Bool SAL_CALL ClusterRowsImpl::supportsService(const OUString &rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence<OUString> SAL_CALL ClusterRowsImpl::getSupportedServiceNames()
{
    return ClusterRowsImpl_getSupportedServiceNames();
}

// XJob method implementations

Any SAL_CALL ClusterRowsImpl::execute(const Sequence<NamedValue> &rArgs)
{
    writeLog("DEBUG>>> Called execute() : this = %p\n", this);

    ClusterRowsImplInfo aJobInfo;
    OUString aErr = validateGetInfo(rArgs, aJobInfo);
    if (!aErr.isEmpty())
    {
        sal_Int16 nArgPos = 0;
        if (aErr.startsWith("Listener"))
            nArgPos = 1;

        throw IllegalArgumentException(
            aErr,
            // resolve to XInterface reference:
            static_cast<::cppu::OWeakObject *>(this),
            nArgPos); // argument pos
    }

    if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq"))
        clusterRows(aJobInfo, 0);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq2"))
        clusterRows(aJobInfo, 2);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq3"))
        clusterRows(aJobInfo, 3);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq4"))
        clusterRows(aJobInfo, 4);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq5"))
        clusterRows(aJobInfo, 5);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq6"))
        clusterRows(aJobInfo, 6);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq7"))
        clusterRows(aJobInfo, 7);
    else if (aJobInfo.aEventName.equalsAscii("onClusterRowsReq8"))
        clusterRows(aJobInfo, 8);

    bool bIsDispatch = aJobInfo.aEnvType.equalsAscii("DISPATCH");
    Sequence<NamedValue> aReturn((bIsDispatch ? 1 : 0));

    if (bIsDispatch)
    {
        aReturn[0].Name = "SendDispatchResult";
        DispatchResultEvent aResultEvent;
        aResultEvent.Source = (cppu::OWeakObject *)this;
        aResultEvent.State = DispatchResultState::SUCCESS;
        aResultEvent.Result <<= true;
        aReturn[0].Value <<= aResultEvent;
    }

    return makeAny(aReturn);
}

OUString ClusterRowsImpl::validateGetInfo(const Sequence<NamedValue> &rArgs,
                                          ClusterRowsImpl::ClusterRowsImplInfo &rJobInfo)
{
    // Extract all sublists from rArgs.
    Sequence<NamedValue> aGenericConfig;
    Sequence<NamedValue> aEnvironment;

    sal_Int32 nNumNVs = rArgs.getLength();
    for (sal_Int32 nIdx = 0; nIdx < nNumNVs; ++nIdx)
    {
        if (rArgs[nIdx].Name.equalsAscii("Config"))
            rArgs[nIdx].Value >>= aGenericConfig;
        else if (rArgs[nIdx].Name.equalsAscii("Environment"))
            rArgs[nIdx].Value >>= aEnvironment;
    }

    // Analyze the environment info. This sub list is the only guaranteed one!
    if (!aEnvironment.hasElements())
        return OUString("Args : no environment");

    sal_Int32 nNumEnvEntries = aEnvironment.getLength();
    for (sal_Int32 nIdx = 0; nIdx < nNumEnvEntries; ++nIdx)
    {
        if (aEnvironment[nIdx].Name.equalsAscii("EnvType"))
            aEnvironment[nIdx].Value >>= rJobInfo.aEnvType;

        else if (aEnvironment[nIdx].Name.equalsAscii("EventName"))
            aEnvironment[nIdx].Value >>= rJobInfo.aEventName;

        else if (aEnvironment[nIdx].Name.equalsAscii("Frame"))
            aEnvironment[nIdx].Value >>= rJobInfo.xFrame;
    }

    // Further the environment property "EnvType" is required as minimum.

    if (rJobInfo.aEnvType.isEmpty() ||
        ((!rJobInfo.aEnvType.equalsAscii("EXECUTOR")) &&
         (!rJobInfo.aEnvType.equalsAscii("DISPATCH"))))
        return OUString("Args : \"" + rJobInfo.aEnvType + "\" isn't a valid value for EnvType");

    // Analyze the set of shared config data.
    if (aGenericConfig.hasElements())
    {
        sal_Int32 nNumGenCfgEntries = aGenericConfig.getLength();
        for (sal_Int32 nIdx = 0; nIdx < nNumGenCfgEntries; ++nIdx)
            if (aGenericConfig[nIdx].Name.equalsAscii("Alias"))
                aGenericConfig[nIdx].Value >>= rJobInfo.aAlias;
    }

    return OUString("");
}

void ClusterRowsImpl::clusterRows(const ClusterRowsImpl::ClusterRowsImplInfo &rJobInfo, const sal_Int32 nUserNumClusters)
{
    TimePerf aTotal("clusterRows");

    if (!rJobInfo.xFrame.is())
    {
        logError("clusterRows : Frame passed is null, cannot color data !");
        return;
    }
    Reference<XModel> xModel = getModel(rJobInfo.xFrame);
    if (!xModel.is())
    {
        logError("colorData : xModel is invalid");
        return;
    }

    Reference<XUndoManagerSupplier> xUndoSupplier(xModel, UNO_QUERY);
    Reference<XUndoManager> xUndoMgr;
    if (xUndoSupplier.is())
        xUndoMgr = xUndoSupplier->getUndoManager();

    if (xUndoMgr.is())
        xUndoMgr->enterUndoContext(OUString("ClusterRowsImpl_UNDO"));

    CellRangeAddress aRange;
    TimePerf aPerfGetDataRange("getDataRange");
    bool bGotRange = getDataRange(xModel, aRange);
    aPerfGetDataRange.Stop();
    if (!bGotRange)
    {
        logError("colorData : Could not get data range !");
        return;
    }
    sal_Int32 nNumCols = aRange.EndColumn - aRange.StartColumn + 1;
    sal_Int32 nNumRows = aRange.EndRow - aRange.StartRow + 1;
    writeLog("DEBUG>>> nNumCols = %d, nNumRows = %d\n", nNumCols, nNumRows);

    Reference<XSpreadsheet> xSheet = getSheet(xModel, aRange.Sheet);
    TimePerf aPerfColor("clusterColorRows");
    sal_Bool bOK = clusterColorRows(xSheet, aRange, nUserNumClusters);
    aPerfColor.Stop();
    if (!bOK)
    {
        logError("clusterRows : clusterColorRows() failed\n");
        return;
    }

    if (xUndoMgr.is())
        xUndoMgr->leaveUndoContext();

    aTotal.Stop();
}

void logError(const char *pStr)
{
    writeLog(pStr);
}

Reference<XModel> getModel(const Reference<XFrame> &rxFrame)
{
    Reference<XModel> xModel;
    if (!rxFrame.is())
        return xModel;

    Reference<XController> xCtrl = rxFrame->getController();
    if (!xCtrl.is())
    {
        logError("getModel : xCtrl is invalid");
        return xModel;
    }

    xModel = xCtrl->getModel();
    return xModel;
}

bool getDataRange(const Reference<XModel> &rxModel, CellRangeAddress &rRangeExtended)
{
    Reference<XCellRangeAddressable> xRange(rxModel->getCurrentSelection(), UNO_QUERY);
    if (!xRange.is())
    {
        logError("getDataRange : Could not get simple data range !");
        return false;
    }

    CellRangeAddress aRange = xRange->getRangeAddress();

    Reference<XSpreadsheet> xSheet = getSheet(rxModel, aRange.Sheet);
    if (!xSheet.is())
    {
        logError("getDataRange : Could not get sheet !");
        return false;
    }

    rRangeExtended.Sheet = aRange.Sheet;
    rRangeExtended.StartColumn = aRange.StartColumn;
    rRangeExtended.EndColumn = aRange.EndColumn;
    rRangeExtended.StartRow = aRange.StartRow;
    rRangeExtended.EndRow = aRange.EndRow;

    shrinkRangeToData(xSheet, rRangeExtended);
    expandRangeToData(xSheet, rRangeExtended);
    excludeResultColumns(xSheet, rRangeExtended);

    return true;
}

Reference<XSpreadsheet> getSheet(const Reference<XModel> &rxModel, const sal_Int32 nSheet)
{
    Reference<XSpreadsheet> xSpreadsheet;

    Reference<XSpreadsheetDocument> xSpreadsheetDocument(rxModel, UNO_QUERY);
    if (!xSpreadsheetDocument.is())
    {
        logError("getSheet : Cannot get xSpreadsheetDocument");
        return xSpreadsheet;
    }

    Reference<XIndexAccess> xSpreadsheets(xSpreadsheetDocument->getSheets(), UNO_QUERY);
    if (!xSpreadsheets.is())
    {
        logError("getSheet : Cannot get xSpreadsheets");
        return xSpreadsheet;
    }
    Any aSheet = xSpreadsheets->getByIndex(nSheet);
    xSpreadsheet = Reference<XSpreadsheet>(aSheet, UNO_QUERY);
    return xSpreadsheet;
}

sal_Bool clusterColorRows(const Reference<XSpreadsheet> &rxSheet, const CellRangeAddress &rRange, const sal_Int32 nUserNumClusters)
{
    sal_Int32 nNumCols = rRange.EndColumn - rRange.StartColumn + 1;
    sal_Int32 nNumRows = rRange.EndRow - rRange.StartRow; // Don't count the header
    if (nNumRows < 10)
    {
        writeLog("DEBUG>>> Too few samples(%d) in the table, need at least 10.\n", nNumRows);
        return false;
    }

    std::vector<bool> aIsColComplete(nNumCols);
    std::vector<DataType> aColType(nNumCols);
    std::vector<std::vector<sal_Int32>> aCol2BlankRowIdx(nNumCols);

    TimePerf aPerfPass1("clusterColorRows_pass1");
    try
    {
        DataType aType;

        for (sal_Int32 nCol = rRange.StartColumn, nColIdx = 0; nCol <= rRange.EndColumn; ++nCol, ++nColIdx)
        {
            aType = DataType::INTEGER;
            bool bIsComplete = true;
            std::vector<sal_Int32> aBlankRowIdx;
            double fMin = 1.0E10, fMax = -1.0E10;
            for (sal_Int32 nRow = rRange.StartRow + 1, nRowIdx = 0; nRow <= rRange.EndRow; ++nRow, ++nRowIdx)
            {
                Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
                if (!xCell.is())
                    logError("getColColors : xCell is invalid.");
                else
                {
                    if (xCell->getType() == CellContentType_TEXT)
                    {
                        aType = DataType::STRING;
                    }
                    else if (xCell->getType() == CellContentType_VALUE && aType == DataType::INTEGER)
                    {
                        double fVal = xCell->getValue();
                        if (fVal != static_cast<double>(static_cast<sal_Int64>(fVal)))
                            aType = DataType::DOUBLE;
                        if (aType == DataType::INTEGER)
                        {
                            fMin = (fMin > fVal) ? fVal : fMin;
                            fMax = (fMax < fVal) ? fVal : fMax;
                        }
                    }
                    else if (xCell->getType() == CellContentType_EMPTY)
                    {
                        bIsComplete = false;
                        aBlankRowIdx.push_back(nRowIdx);
                    }
                }
            }

            if (aType == DataType::INTEGER && (fMax - fMin) > 100.0)
                aType = DataType::DOUBLE;

            aIsColComplete[nColIdx] = bIsComplete;
            aColType[nColIdx] = aType;
            aCol2BlankRowIdx[nColIdx] = std::move(aBlankRowIdx);

            writeLog("DEBUG>>> col = %d, Type = %d, isComplete = %d\n", nColIdx, aType, int(bIsComplete));
        }
    }
    catch (Exception &e)
    {
        writeLog("DEBUG>>> clusterColorRows : caught UNO exception: %s\n",
                OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US).getStr());
        return false;
    }

    aPerfPass1.Stop();

    TimePerf aPerfPass2("clusterColorRows_pass2");

    TimePerf aPerfAcquire("dataAcquire");
    Reference<XCellRangeData> xData(
        rxSheet->getCellRangeByPosition(rRange.StartColumn, rRange.StartRow + 1, rRange.EndColumn, rRange.EndRow),
        UNO_QUERY);
    Sequence<Sequence<Any>> aDataArray = xData->getDataArray();
    aPerfAcquire.Stop();

    TimePerf aPerfPreprocess("dataPreprocess");
    flagEmptyEntries(aDataArray, aColType, aCol2BlankRowIdx);
    imputeAllColumns(aDataArray, aColType, aCol2BlankRowIdx);
    std::vector<std::pair<double, double>> aFeatureScales(nNumCols);
    calculateFeatureScales(aDataArray, aColType, aFeatureScales);
    for (sal_Int32 nColIdx = 0; nColIdx < nNumCols; ++nColIdx)
        writeLog("DEBUG>>> col %d has type %d, mean = %.4f, std = %.5f\n", nColIdx, aColType[nColIdx], aFeatureScales[nColIdx].first, aFeatureScales[nColIdx].second);
    aPerfPreprocess.Stop();

    TimePerf aPerfCompute("computeClusters");
    std::vector<sal_Int32> aClusterLabels(nNumRows);
    std::vector<double> aLabelConfidence(nNumRows);
    sal_Int32 nNumClusters = nUserNumClusters;
    bool bClusterOK = getClusterLabels(aDataArray, aColType, aFeatureScales, aClusterLabels, aLabelConfidence, nNumClusters);
    if (!bClusterOK)
    {
        logError("clusterColorRows : Clustering failed !");
        return false;
    }

    std::vector<sal_Int32> aRowColors(nNumRows);
    getRowColors(aClusterLabels, aLabelConfidence, nNumClusters, aRowColors);
    aPerfCompute.Stop();

    TimePerf aPerfWriteOutput("WriteOutput");
    Reference<XCellRange> xCellRange(rxSheet, UNO_QUERY);
    if (!xCellRange.is())
    {
        logError("setColColors : cannot get xCellRange");
        return false;
    }
    if (rRange.EndColumn + 2 <= MAXCOL)
    {
        Reference<XCell> xCell = rxSheet->getCellByPosition(rRange.EndColumn + 1, rRange.StartRow);
        xCell->setFormula("ClusterId");
        xCell = rxSheet->getCellByPosition(rRange.EndColumn + 2, rRange.StartRow);
        xCell->setFormula("Confidence");
    }
    for (sal_Int32 nRowIdx = 0; nRowIdx < nNumRows; ++nRowIdx)
    {
        sal_Int32 nRow = rRange.StartRow + 1 + nRowIdx;
        Reference<XCellRange> xThisRow = xCellRange->getCellRangeByPosition(rRange.StartColumn, nRow, rRange.EndColumn, nRow);
        Reference<XPropertySet> xPropSet(xThisRow, UNO_QUERY);
        if (!xPropSet.is())
        {
            writeLog("DEBUG>>> clusterColorRows : Cannot get xPropSet for Row = %d !\n", nRow);
            continue;
        }
        xPropSet->setPropertyValue("CellBackColor", makeAny(aRowColors[nRowIdx]));
        Reference<XCell> xCell = rxSheet->getCellByPosition(rRange.EndColumn + 1, nRow);
        xCell->setValue(static_cast<double>(aClusterLabels[nRowIdx]));
        xCell = rxSheet->getCellByPosition(rRange.EndColumn + 2, nRow);
        xCell->setValue(aLabelConfidence[nRowIdx]);
    }
    aPerfWriteOutput.Stop();
    aPerfPass2.Stop();

    return true;
}

bool getClusterLabels(const Sequence<Sequence<Any>> &rDataArray,
                      const std::vector<DataType> &rColType,
                      const std::vector<std::pair<double, double>> &rFeatureScales,
                      std::vector<sal_Int32> &rClusterLabels,
                      std::vector<double> &rLabelConfidence,
                      sal_Int32 &rNumClusters)
{
    //performNoOpClustering( rDataArray, rColType, rFeatureScales, rClusterLabels, rLabelConfidence, rNumClusters );
    performEMClustering(rDataArray, rColType, rFeatureScales, rClusterLabels, rLabelConfidence, rNumClusters);
    return true;
}


// This is the service name an Add-On has to implement
#define ADDIN_BASE_SERVICE_NAME "com.sun.star.sheet.AddIn"
#define ADDIN_SERVICE_NAME "com.github.dennisfrancis.GMMCluster"

// Helper functions for the implementation of UNO component interfaces.
OUString GMMClusterImpl_getImplementationName()
{
    return OUString ( ADDIN_IMPLEMENTATION_NAME );
}

Sequence< OUString > SAL_CALL GMMClusterImpl_getSupportedServiceNames()
{
    Sequence < OUString > aRet(2);
    OUString* pArray = aRet.getArray();
    pArray[0] =  OUString ( ADDIN_BASE_SERVICE_NAME );
    pArray[1] =  OUString ( ADDIN_SERVICE_NAME );
    return aRet;
}

Reference< XInterface > SAL_CALL GMMClusterImpl_createInstance( const Reference< XComponentContext > & rContext)
{
    return (cppu::OWeakObject*) new GMMClusterImpl();
}

OUString GMMClusterImpl::getServiceName()
{
    return ADDIN_SERVICE_NAME;
}

// Implementation of the recommended/mandatory interfaces of a UNO component.
// XServiceInfo
OUString SAL_CALL GMMClusterImpl::getImplementationName()
{
    return GMMClusterImpl_getImplementationName();
}

sal_Bool SAL_CALL GMMClusterImpl::supportsService( const OUString& rServiceName )
{
    return ( rServiceName == ADDIN_BASE_SERVICE_NAME || rServiceName == ADDIN_SERVICE_NAME );
}

Sequence< OUString > SAL_CALL GMMClusterImpl::getSupportedServiceNames(  )
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
    },
};

const OUString GMMClusterImpl::aArgumentDescriptions[NUMFUNCTIONS][NUMARGS] = {
    {
        "cell range of data to be clustered",
        "number of clusters (optional)",
        "number of epochs (optional)",
    },
};

sal_Int32 GMMClusterImpl::getFunctionID( const OUString aProgrammaticFunctionName ) const
{
    writeLog("getFunctionID : aProgrammat = %s\n", aProgrammaticFunctionName.toUtf8().getStr());fflush(stdout);
    for ( sal_Int32 nIdx = 0; nIdx < nNumFunctions; ++nIdx )
        if ( aProgrammaticFunctionName == aFunctionNames[nIdx] )
            return nIdx;
    return -1;
}

Sequence< Sequence< double > > SAL_CALL
GMMClusterImpl::gmmCluster(const Sequence < Sequence < Any > >& dataConst,
    const Any& numClusters, const Any& numEpochs)
{
    if (!dataConst.getLength())
        return Sequence< Sequence<double> >();

    const sal_Int32 nNumRows = dataConst.getLength();
    const sal_Int32 nNumCols = dataConst[0].getLength();
    Sequence< Sequence< Any > > data(dataConst.getLength());
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
        writeLog("DEBUG>>> col %d has type %d, mean = %.4f, std = %.5f\n",
            nColIdx, aColType[nColIdx], aFeatureScales[nColIdx].first,
            aFeatureScales[nColIdx].second);
    }
    aPerfPreprocess.Stop();

    TimePerf aPerfCompute("computeClusters");
    std::vector<sal_Int32> aClusterLabels(nNumRows);
    std::vector<double> aLabelConfidence(nNumRows);
    sal_Int32 nNumClusters = 0;
    sal_Int32 nNumEpochs = static_cast<sal_Int32>(MAXEPOCHS);
    numClusters >>= nNumClusters;
    numEpochs >>= nNumEpochs;
    performEMClustering(data, aColType, aFeatureScales, aClusterLabels, aLabelConfidence, nNumClusters, nNumEpochs);
    Sequence< Sequence <double> > aSeq(nNumRows);
    for (sal_Int32 nRow = 0; nRow < nNumRows; ++nRow)
    {
        aSeq[nRow].realloc(2);
        aSeq[nRow][0] = static_cast<double>(aClusterLabels[nRow]),
        aSeq[nRow][1] = aLabelConfidence[nRow];
    }

    return aSeq;
}

OUString GMMClusterImpl::getProgrammaticFuntionName( const OUString& aDisplayName )
{
    writeLog("getProgrammaticFuntionName : aDisplayName = %s\n", aDisplayName.toUtf8().getStr());fflush(stdout);
    for ( sal_Int32 nIdx = 0; nIdx < nNumFunctions; ++nIdx )
        if ( aDisplayName == aDisplayFunctionNames[nIdx] )
            return aFunctionNames[nIdx];
    return "";
}

OUString GMMClusterImpl::getDisplayFunctionName( const OUString& aProgrammaticName )
{
    writeLog("getProgrammaticFuntionName : aPro = %s\n", aProgrammaticName.toUtf8().getStr());fflush(stdout);
    sal_Int32 nFIdx = getFunctionID( aProgrammaticName );
    return ( nFIdx == -1 ) ? "" : aDisplayFunctionNames[nFIdx];
}

OUString GMMClusterImpl::getFunctionDescription( const OUString& aProgrammaticName )
{
    sal_Int32 nFIdx = getFunctionID( aProgrammaticName );
    return ( nFIdx == -1 ) ? "" : aDescriptions[nFIdx];
}

OUString GMMClusterImpl::getDisplayArgumentName( const OUString& aProgrammaticFunctionName, sal_Int32 nArgument )
{
    sal_Int32 nFIdx = getFunctionID( aProgrammaticFunctionName );
    return ( nFIdx == -1 || (nArgument < 0 || nArgument >= NUMARGS) ) ? "" : aArgumentNames[nFIdx][nArgument];
}

OUString GMMClusterImpl::getArgumentDescription( const OUString& aProgrammaticFunctionName, sal_Int32 nArgument )
{
    sal_Int32 nFIdx = getFunctionID( aProgrammaticFunctionName );
    return ( nFIdx == -1 || (nArgument < 0 || nArgument >= NUMARGS) ) ? "" : aArgumentDescriptions[nFIdx][nArgument];
}

OUString GMMClusterImpl::getProgrammaticCategoryName( const OUString& aProgrammaticFunctionName )
{
    return "Add-In";
}

OUString GMMClusterImpl::getDisplayCategoryName( const OUString& aProgrammaticFunctionName )
{
    return "Add-In";
}

