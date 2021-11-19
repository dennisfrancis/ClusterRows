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

#include "helper.hxx"
#include "logging.hxx"
#include "range.hxx"

#include <com/sun/star/uno/XComponentContext.hpp>

#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XModel.hpp>

#include <com/sun/star/sheet/XCellRangeAddressable.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>

#include <com/sun/star/table/CellRangeAddress.hpp>

#include <com/sun/star/container/XIndexAccess.hpp>

#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/awt/WindowDescriptor.hpp>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>

using namespace com::sun::star::frame;
using namespace com::sun::star::uno;
using namespace com::sun::star::sheet;
using namespace com::sun::star::container;
using namespace com::sun::star::awt;
using namespace com::sun::star::table;

using rtl::OUString;
using namespace range;

Reference<XModel> helper::getModel(const Reference<XFrame>& rxFrame)
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

bool helper::getDataRange(const Reference<XModel>& rxModel, CellRangeAddress& rRangeExtended)
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

    constexpr sal_Int32 nMaxRowsInSelection = 8000;
    constexpr sal_Int32 nMaxColsInSelection = 100;
    sal_Int32 nNumRows = rRangeExtended.EndRow - rRangeExtended.StartRow + 1;
    sal_Int32 nNumCols = rRangeExtended.EndColumn - rRangeExtended.StartColumn + 1;
    if (nNumRows > nMaxRowsInSelection)
        rRangeExtended.EndRow = rRangeExtended.StartRow + nMaxRowsInSelection - 1;
    if (nNumCols > nMaxColsInSelection)
        rRangeExtended.EndColumn = rRangeExtended.StartColumn + nMaxColsInSelection - 1;

    shrinkRangeToData(xSheet, rRangeExtended);
    if (rangeIsSingleCell(rRangeExtended))
    {
        bool bEmpty = isCellEmpty(xSheet, rRangeExtended.StartColumn, rRangeExtended.StartRow);
        bool bEmptyAbove = true;
        if (rRangeExtended.StartRow)
            bEmptyAbove
                = isCellEmpty(xSheet, rRangeExtended.StartColumn, rRangeExtended.StartRow - 1);
        bool bEmptyBelow = true;
        if (rRangeExtended.StartRow < MAXROW)
            bEmptyBelow
                = isCellEmpty(xSheet, rRangeExtended.StartColumn, rRangeExtended.StartRow + 1);

        if (bEmpty)
        {
            if (!bEmptyBelow)
            {
                ++rRangeExtended.StartRow;
                ++rRangeExtended.EndRow;
            }
            else if (!bEmptyAbove)
            {
                --rRangeExtended.StartRow;
                --rRangeExtended.EndRow;
            }
            else
            {
                return true;
            }
        }
    }

    writeLog("after shrink: range = %s\n", getCellRangeRepr(rRangeExtended).toUtf8().getStr());
    expandRangeToData(xSheet, rRangeExtended);
    writeLog("after expand: range = %s\n", getCellRangeRepr(rRangeExtended).toUtf8().getStr());
    excludeResultColumns(xSheet, rRangeExtended);

    return true;
}

Reference<XSpreadsheet> helper::getSheet(const Reference<XModel>& rxModel, const sal_Int32 nSheet)
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

void helper::showErrorMessage(const Reference<XFrame>& xFrame, const OUString& aTitle,
                              const OUString& aMsgText, const Reference<XComponentContext>& xCtxt)
{
    Reference<XToolkit> xToolkit(
        xCtxt->getServiceManager()->createInstanceWithContext("com.sun.star.awt.Toolkit", xCtxt),
        UNO_QUERY);
    if (!xToolkit.is())
    {
        writeLog("showErrorMessage: Cannot get instance of XToolkit!\n");
        return;
    }

    if (!xFrame.is())
    {
        writeLog("showErrorMessage: XFrame passes is empty!\n");
        return;
    }

    // describe window properties.
    WindowDescriptor aDescriptor;
    aDescriptor.Type = WindowClass_MODALTOP;
    aDescriptor.WindowServiceName = OUString(RTL_CONSTASCII_USTRINGPARAM("infobox"));
    aDescriptor.ParentIndex = -1;
    aDescriptor.Parent = Reference<XWindowPeer>(xFrame->getContainerWindow(), UNO_QUERY);
    aDescriptor.Bounds = Rectangle(300, 200, 300, 200);
    aDescriptor.WindowAttributes
        = WindowAttribute::BORDER | WindowAttribute::MOVEABLE | WindowAttribute::CLOSEABLE;

    Reference<XWindowPeer> xPeer = xToolkit->createWindow(aDescriptor);
    if (xPeer.is())
    {
        Reference<XMessageBox> xMsgBox(xPeer, UNO_QUERY);
        if (xMsgBox.is())
        {
            xMsgBox->setCaptionText(aTitle);
            xMsgBox->setMessageText(aMsgText);
            xMsgBox->execute();
        }
    }
}

OUString helper::getStyleName(sal_Int32 nClusterIdx, sal_Int32 nNumClusters)
{
    return OUString("ClusterRows_N") + OUString::number(nNumClusters) + "_Cluster_"
           + OUString::number(nClusterIdx);
}
