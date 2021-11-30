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

#include "cluster.hxx"
#include "perf.hxx"

#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>

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
#include <com/sun/star/sheet/XArrayFormulaRange.hpp>
#include <com/sun/star/sheet/XSheetConditionalEntries.hpp>
#include <com/sun/star/sheet/ConditionOperator.hpp>

#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

#include <com/sun/star/table/XCellRange.hpp>

#include <com/sun/star/util/Color.hpp>

#include <com/sun/star/document/XUndoManager.hpp>
#include <com/sun/star/document/XUndoManagerSupplier.hpp>

#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>

#include <cppuhelper/supportsservice.hxx>

#include <vector>
#include <chrono>
#include <thread>

#include "helper.hxx"
#include "DialogHelper.hxx"
#include "range.hxx"
#include "preprocess.hxx"
#include "colorgen.hxx"
#include "em.hxx"

using namespace com::sun::star::awt;
using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::sheet;
using namespace com::sun::star::table;
using namespace com::sun::star::awt;
using namespace com::sun::star::style;

using com::sun::star::beans::NamedValue;
using com::sun::star::beans::XPropertySet;
using com::sun::star::beans::PropertyValue;

using com::sun::star::container::XIndexAccess;
using com::sun::star::container::XNameAccess;
using com::sun::star::container::XNameContainer;

using com::sun::star::document::XUndoManager;
using com::sun::star::document::XUndoManagerSupplier;

using com::sun::star::frame::DispatchResultEvent;
using com::sun::star::lang::IllegalArgumentException;
using com::sun::star::lang::XMultiServiceFactory;
using com::sun::star::util::Color;

using rtl::OUString;

using namespace helper;
using namespace range;
using namespace preprocess;
using namespace em;

// This is the service name an Add-On has to implement
#define SERVICE_NAME "com.sun.star.task.Job"

// Helper functions for the implementation of UNO component interfaces.
OUString ClusterRowsImpl_getImplementationName() { return OUString(IMPLEMENTATION_NAME); }

Sequence<OUString> SAL_CALL ClusterRowsImpl_getSupportedServiceNames()
{
    Sequence<OUString> aRet(1);
    OUString* pArray = aRet.getArray();
    pArray[0] = OUString(SERVICE_NAME);
    return aRet;
}

Reference<XInterface>
    SAL_CALL ClusterRowsImpl_createInstance(const Reference<XComponentContext>& rContext)
{
    return (::cppu::OWeakObject*)new ClusterRowsImpl(rContext);
}

ClusterRowsImpl::ClusterRowsImpl(const Reference<XComponentContext>& rxContext)
    : mxContext(rxContext)
{
    maDataRange.StartColumn = maDataRange.EndColumn = -1;
    maDataRange.StartRow = maDataRange.EndRow = -1;
    mbHasHeader = false;
    maParams.mnNumClusters = -1;
    maParams.mnNumEpochs = -1;
    maParams.mnNumIterations = -1;
    maParams.mbColorClusters = false;
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

sal_Bool SAL_CALL ClusterRowsImpl::supportsService(const OUString& rServiceName)
{
    return ::cppu::supportsService(this, rServiceName);
}

Sequence<OUString> SAL_CALL ClusterRowsImpl::getSupportedServiceNames()
{
    return ClusterRowsImpl_getSupportedServiceNames();
}

// XJob method implementations

Any SAL_CALL ClusterRowsImpl::execute(const Sequence<NamedValue>& rArgs)
{
    writeLog("DEBUG>>> Called execute() : this = %p\n", this);

    ClusterRowsImplInfo aJobInfo;
    OUString aErr = validateGetInfo(rArgs, aJobInfo);
    if (!aErr.isEmpty())
    {
        sal_Int16 nArgPos = 0;
        if (aErr.startsWith("Listener"))
            nArgPos = 1;

        throw IllegalArgumentException(aErr,
                                       // resolve to XInterface reference:
                                       static_cast<::cppu::OWeakObject*>(this),
                                       nArgPos); // argument pos
    }

    if (aJobInfo.aEventName == "onClusterRowsReqDialog")
    {
        launchClusterDialog(aJobInfo);
    }

    bool bIsDispatch = aJobInfo.aEnvType.equalsAscii("DISPATCH");
    Sequence<NamedValue> aReturn((bIsDispatch ? 1 : 0));

    if (bIsDispatch)
    {
        aReturn[0].Name = "SendDispatchResult";
        DispatchResultEvent aResultEvent;
        aResultEvent.Source = (::cppu::OWeakObject*)this;
        aResultEvent.State = DispatchResultState::SUCCESS;
        aResultEvent.Result <<= true;
        aReturn[0].Value <<= aResultEvent;
    }

    return makeAny(aReturn);
}

void ClusterRowsImpl::launchClusterDialog(const ClusterRowsImplInfo& aJobInfo)
{
    CellRangeAddress aRange;
    bool bGotRange = calcDataRange(aJobInfo, aRange);
    if (!bGotRange)
    {
        showErrorMessage(aJobInfo.xFrame, "ClusterRows",
                         "Could not calculate data range from cell cursor location!", mxContext);
        return;
    }

    if (aRange.EndColumn + 2 > MAXCOL)
    {
        // No space to write results!
        showErrorMessage(aJobInfo.xFrame, "ClusterRows",
                         "No columns to the right of the data table to write results!", mxContext);
        return;
    }

    sal_Int32 nNumRows = aRange.EndRow - aRange.StartRow + 1;
    Reference<XModel> xModel = getModel(aJobInfo.xFrame);

    mxDoc = Reference<XSpreadsheetDocument>(xModel, UNO_QUERY);
    if (!mxDoc.is())
    {
        writeLog("launchClusterDialog: Cannot get XSpreadsheetDocument from XModel!\n");
        return;
    }

    Reference<XSpreadsheet> xSheet = getSheet(xModel, aRange.Sheet);
    mxSheet = xSheet;
    maDataRange = aRange;
    mbHasHeader = false;
    if (hasHeader(xSheet, aRange))
    {
        mbHasHeader = true;
        --nNumRows;
        ++maDataRange.StartRow;
    }

    if (nNumRows < 10)
    {
        OUString aMsg("Too few samples in the table, need at least 10 rows of data!");
        showErrorMessage(aJobInfo.xFrame, "ClusterRows", aMsg, mxContext);
        return;
    }

    Reference<XDialog> xDialog = dialoghelper::createDialog("ClusterRows.xdl", mxContext, this,
                                                            getCellRangeRepr(maDataRange));
    writeLog("onClusterRowsReqDialog: executing dialog!\n");
    xDialog->execute();
}

// XDialogEventHandler methods
sal_Bool
ClusterRowsImpl::callHandlerMethod(const Reference<::com::sun::star::awt::XDialog>& xDialog,
                                   const Any& /*eventObject*/, const OUString& methodName)
{
    return dialoghelper::onAction(methodName, xDialog, [this](const ClusterParams& aParams) {
        maParams = aParams;
        writeLog("Range = %s, mbHasHeader = %d\n", getCellRangeRepr(maDataRange).toUtf8().getStr(),
                 mbHasHeader);
        writeResults();
    });
}

void ClusterRowsImpl::writeResults()
{
    sal_Int32 nColStart = maDataRange.EndColumn + 1;
    sal_Int32 nColEnd = nColStart + 1;
    sal_Int32 nRowStart = maDataRange.StartRow;
    sal_Int32 nRowEnd = maDataRange.EndRow;
    Reference<XCellRange> xRange(
        mxSheet->getCellRangeByPosition(nColStart, nRowStart, nColEnd, nRowEnd));
    Reference<XArrayFormulaRange> xAFR(xRange, UNO_QUERY);
    if (!xAFR.is())
    {
        writeLog("onAction callback: FAILED: Cannot get XArrayFormulaRange from XCellRange for "
                 "writing the array formula!");
        return;
    }

    Reference<XUndoManagerSupplier> xUndoSupplier(mxDoc, UNO_QUERY);
    Reference<XUndoManager> xUndoMgr;
    if (xUndoSupplier.is())
        xUndoMgr = xUndoSupplier->getUndoManager();

    if (xUndoMgr.is())
        xUndoMgr->enterUndoContext(OUString("ClusterRowsImpl_UNDO"));

    xAFR->setArrayFormula(
        "=COM.GITHUB.DENNISFRANCIS.GMMCLUSTER.GMMCLUSTER(" + getCellRangeRepr(maDataRange) + ";"
        + OUString::number(maParams.mnNumClusters) + ";" + OUString::number(maParams.mnNumEpochs)
        + ";" + OUString::number(maParams.mnNumIterations) + ")");

    if (mbHasHeader)
    {
        Reference<XCell> xCell = mxSheet->getCellByPosition(nColStart, nRowStart - 1);
        xCell->setFormula("ClusterId");
        xCell = mxSheet->getCellByPosition(nColStart + 1, nRowStart - 1);
        xCell->setFormula("Confidence");
    }

    updateNumClusters();

    if (maParams.mbColorClusters)
    {
        addClusterStyles();
        colorClusterData();
    }

    if (xUndoMgr.is())
        xUndoMgr->leaveUndoContext();
}

/// Find number of clusters in auto mode.
void ClusterRowsImpl::updateNumClusters()
{
    if (maParams.mnNumClusters != 0)
        return;

    sal_Int32 nMaxClusterIdx = findMaxInColumn(mxSheet, maDataRange.EndColumn + 1,
                                               maDataRange.StartRow, maDataRange.EndRow);

    if (nMaxClusterIdx != -1)
        maParams.mnNumClusters = nMaxClusterIdx + 1;
}

void ClusterRowsImpl::colorClusterData() const
{
    Reference<XCellRange> xRange = mxSheet->getCellRangeByPosition(
        maDataRange.StartColumn, maDataRange.StartRow, maDataRange.EndColumn, maDataRange.EndRow);

    Reference<XPropertySet> xPropSet(xRange, UNO_QUERY);
    Reference<XSheetConditionalEntries> xEntries;
    Any aAny = xPropSet->getPropertyValue("ConditionalFormat");
    if (!(aAny >>= xEntries))
    {
        writeLog(
            "colorClusterData: Cannot get XSheetConditionalEntries from Any(ConditionalFormat)");
        return;
    }

    xEntries->clear();

    OUString aRefCell = OUString("$")
                        + getCellAddressRepr(maDataRange.EndColumn + 1,
                                             0 /* relative row w.r.t data table's top row*/);
    writeLog("colorClusterData: aRefCell = %s\n", aRefCell.toUtf8().getStr());

    for (sal_Int32 nClusterIdx = 0; nClusterIdx < maParams.mnNumClusters; ++nClusterIdx)
    {
        Sequence<PropertyValue> aConds(3);

        aConds[0].Name = "Operator";
        aConds[0].Value = Any(ConditionOperator_FORMULA);

        aConds[1].Name = "Formula1";
        aConds[1].Value = Any(aRefCell + " = " + OUString::number(nClusterIdx));

        aConds[2].Name = "StyleName";
        aConds[2].Value = Any(getStyleName(nClusterIdx, maParams.mnNumClusters));

        xEntries->addNew(aConds);
    }

    xPropSet->setPropertyValue("ConditionalFormat", Any(xEntries));
}

void ClusterRowsImpl::addClusterStyles() const
{
    Reference<XStyleFamiliesSupplier> xSFSupplier(mxDoc, UNO_QUERY);
    Reference<XNameAccess> xFamiliesNA(xSFSupplier->getStyleFamilies());
    Reference<XNameContainer> xCellStylesNA(xFamiliesNA->getByName("CellStyles"), UNO_QUERY);

    Reference<XMultiServiceFactory> xServiceManager(mxDoc, UNO_QUERY);
    std::vector<sal_Int32> aClusterColors(maParams.mnNumClusters);
    getClusterColors(maParams.mnNumClusters, aClusterColors);
    writeLog("addClusterStyles: Calculated %d colors\n", aClusterColors.size());

    for (sal_Int32 nClusterIdx = 0; nClusterIdx < maParams.mnNumClusters; ++nClusterIdx)
    {
        OUString aClusterStyle = getStyleName(nClusterIdx, maParams.mnNumClusters);
        Reference<XInterface> xClusterStyle;
        if (xCellStylesNA->hasByName(aClusterStyle))
        {
            Any aAny = xCellStylesNA->getByName(aClusterStyle);
            aAny >>= xClusterStyle;
        }
        else
        {
            xClusterStyle = xServiceManager->createInstance("com.sun.star.style.CellStyle");
            xCellStylesNA->insertByName(aClusterStyle, Any(xClusterStyle));
        }

        Reference<XPropertySet> xPropSet(xClusterStyle, UNO_QUERY);
        xPropSet->setPropertyValue("CellBackColor", Any(aClusterColors[nClusterIdx]));
        writeLog("addClusterStyles: Added Cell style : %s\n", aClusterStyle.toUtf8().getStr());
    }
}

Sequence<OUString> ClusterRowsImpl::getSupportedMethodNames()
{
    Sequence<OUString> aActions(3);
    aActions[0] = "onOKButtonPress";
    aActions[1] = "onCancelButtonPress";
    aActions[2] = "onInputChange";
    return aActions;
}

OUString ClusterRowsImpl::validateGetInfo(const Sequence<NamedValue>& rArgs,
                                          ClusterRowsImpl::ClusterRowsImplInfo& rJobInfo)
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

    if (rJobInfo.aEnvType.isEmpty()
        || ((!rJobInfo.aEnvType.equalsAscii("EXECUTOR"))
            && (!rJobInfo.aEnvType.equalsAscii("DISPATCH"))))
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

bool ClusterRowsImpl::calcDataRange(const ClusterRowsImplInfo& rJobInfo,
                                    CellRangeAddress& aRange) const
{
    TimePerf aTotal("calcDataRange");
    if (!rJobInfo.xFrame.is())
    {
        logError("calcDataRange : Frame passed is null, cannot color data !");
        return false;
    }
    Reference<XModel> xModel = getModel(rJobInfo.xFrame);
    if (!xModel.is())
    {
        logError("calcDataRange : xModel is invalid");
        return false;
    }

    TimePerf aPerfGetDataRange("getDataRange");
    bool bGotRange = getDataRange(xModel, aRange);
    aPerfGetDataRange.Stop();

    return bGotRange;
}
