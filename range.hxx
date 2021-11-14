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

#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/table/CellContentType.hpp>
#include <vector>
#include "logging.hxx"
#include "datatypes.hxx"

#define MAXROW 1048575
#define MAXCOL 1023

using com::sun::star::sheet::XSpreadsheet;
using com::sun::star::table::CellContentType_EMPTY;
using com::sun::star::table::CellContentType_TEXT;
using com::sun::star::table::CellRangeAddress;
using com::sun::star::table::XCell;
using com::sun::star::uno::Reference;

bool rangeIsSingleCell(const CellRangeAddress& rRange)
{
    return (rRange.StartColumn == rRange.EndColumn) && (rRange.StartRow == rRange.EndRow);
}

bool isCellEmpty(const Reference<XSpreadsheet>& rxSheet, sal_Int32 nCol, sal_Int32 nRow)
{
    Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
    if (!xCell.is())
    {
        writeLog("DEBUG>>> isCellEmpty : xCell(%d, %d) is invalid.\n", nCol, nRow);
        return false;
    }
    return (xCell->getType() == CellContentType_EMPTY);
}

bool isColumnEmpty(const Reference<XSpreadsheet>& rxSheet, sal_Int32 nCol, sal_Int32 nStartRow,
                   sal_Int32 nEndRow)
{
    for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
        if (!isCellEmpty(rxSheet, nCol, nRow))
            return false;

    return true;
}

bool isRowEmpty(const Reference<XSpreadsheet>& rxSheet, sal_Int32 nRow, sal_Int32 nStartCol,
                sal_Int32 nEndCol)
{
    for (sal_Int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
        if (!isCellEmpty(rxSheet, nCol, nRow))
            return false;

    return true;
}

void shrinkRangeToData(const Reference<XSpreadsheet>& rxSheet, CellRangeAddress& rRangeExtended)
{
    sal_Int32 nStartCol = rRangeExtended.StartColumn;
    sal_Int32 nEndCol = rRangeExtended.EndColumn;
    sal_Int32 nStartRow = rRangeExtended.StartRow;
    sal_Int32 nEndRow = rRangeExtended.EndRow;

    bool bStop = false;
    // Shrink nStartCol
    for (sal_Int32 nCol = nStartCol; (nCol <= nEndCol && !bStop); ++nCol)
    {
        bool bColEmpty = isColumnEmpty(rxSheet, nCol, nStartRow, nEndRow);
        if (!bColEmpty)
        {
            bStop = true;
            nStartCol = nCol;
        }
        else if (nCol == nEndCol)
            nStartCol = nEndCol;
    }

    bStop = false;
    // Shrink nEndCol
    for (sal_Int32 nCol = nEndCol; (nCol >= nStartCol && !bStop); --nCol)
    {
        bool bColEmpty = isColumnEmpty(rxSheet, nCol, nStartRow, nEndRow);
        if (!bColEmpty)
        {
            bStop = true;
            nEndCol = nCol;
        }
        else if (nCol == nStartCol)
            nEndCol = nStartCol;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);

    bStop = false;
    // Shrink nStartRow
    for (sal_Int32 nRow = nStartRow; (nRow <= nEndRow && !bStop); ++nRow)
    {
        bool bRowEmpty = isRowEmpty(rxSheet, nRow, nStartCol, nEndCol);
        if (!bRowEmpty)
        {
            bStop = true;
            nStartRow = nRow;
        }
        else if (nRow == nEndRow)
            nStartRow = nEndRow;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);
    //writeLog("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow);

    bStop = false;
    // Shrink nEndRow
    for (sal_Int32 nRow = nEndRow; (nRow >= nStartRow && !bStop); --nRow)
    {
        bool bRowEmpty = isRowEmpty(rxSheet, nRow, nStartCol, nEndCol);
        if (!bRowEmpty)
        {
            bStop = true;
            nEndRow = nRow;
        }
        else if (nRow == nStartRow)
            nEndRow = nStartRow;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);
    //writeLog("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow);

    rRangeExtended.StartRow = nStartRow;
    rRangeExtended.EndRow = nEndRow;
    rRangeExtended.StartColumn = nStartCol;
    rRangeExtended.EndColumn = nEndCol;
}

void expandRangeToData(const Reference<XSpreadsheet>& rxSheet, CellRangeAddress& rRangeExtended)
{
    sal_Int32 nStartCol = rRangeExtended.StartColumn;
    sal_Int32 nEndCol = rRangeExtended.EndColumn;
    sal_Int32 nStartRow = rRangeExtended.StartRow;
    sal_Int32 nEndRow = rRangeExtended.EndRow;

    bool bStop = false;
    // Extend nStartCol
    for (sal_Int32 nCol = nStartCol - 1; (nCol >= 0 && !bStop); --nCol)
    {
        bool bColEmpty = isColumnEmpty(rxSheet, nCol, nStartRow, nEndRow);
        if (bColEmpty)
        {
            bStop = true;
            nStartCol = nCol + 1;
        }
        else if (nCol == 0)
            nStartCol = 0;
    }

    bStop = false;
    // Extend nEndCol
    for (sal_Int32 nCol = nEndCol + 1; (nCol <= MAXCOL && !bStop); ++nCol)
    {
        bool bColEmpty = isColumnEmpty(rxSheet, nCol, nStartRow, nEndRow);
        if (bColEmpty)
        {
            bStop = true;
            nEndCol = nCol - 1;
        }
        else if (nCol == MAXCOL)
            nEndCol = MAXCOL;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);

    bStop = false;
    // Extend nStartRow
    for (sal_Int32 nRow = nStartRow - 1; (nRow >= 0 && !bStop); --nRow)
    {
        bool bRowEmpty = isRowEmpty(rxSheet, nRow, nStartCol, nEndCol);
        if (bRowEmpty)
        {
            bStop = true;
            nStartRow = nRow + 1;
        }
        else if (nRow == 0)
            nStartRow = 0;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);
    //writeLog("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow);

    bStop = false;
    // Extend nEndRow
    for (sal_Int32 nRow = nEndRow + 1; (nRow <= MAXROW && !bStop); ++nRow)
    {
        bool bRowEmpty = isRowEmpty(rxSheet, nRow, nStartCol, nEndCol);
        if (bRowEmpty)
        {
            bStop = true;
            nEndRow = nRow - 1;
        }
        else if (nRow == MAXROW)
            nEndRow = MAXROW;
    }

    //writeLog("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol);
    //writeLog("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow);

    rRangeExtended.StartRow = nStartRow;
    rRangeExtended.EndRow = nEndRow;
    rRangeExtended.StartColumn = nStartCol;
    rRangeExtended.EndColumn = nEndCol;
}

void excludeResultColumns(const Reference<XSpreadsheet>& rxSheet, CellRangeAddress& rRangeExtended)
{
    // Result headers in reverse order ( with rightmost header first )
    std::vector<OUString> aResultHeaders = { "Confidence", "ClusterId" };
    Reference<XCell> xCell;
    for (OUString& aHdr : aResultHeaders)
    {
        xCell = rxSheet->getCellByPosition(rRangeExtended.EndColumn, rRangeExtended.StartRow);
        if (!xCell.is())
        {
            writeLog("DEBUG>>> excludeResultColumns : xCell(%d, %d) is invalid.\n",
                     rRangeExtended.EndColumn, rRangeExtended.StartRow);
            return;
        }
        if (xCell->getFormula() == aHdr)
            --rRangeExtended.EndColumn;
    }
}

bool hasHeader(const Reference<XSpreadsheet>& xSheet, CellRangeAddress& aRange)
{
    for (sal_Int32 nCol = aRange.StartColumn; nCol <= aRange.EndColumn; ++nCol)
    {
        Reference<XCell> xCell = xSheet->getCellByPosition(nCol, aRange.StartRow);
        if (xCell->getType() != CellContentType_TEXT)
            return false;
    }

    return true;
}

OUString getCellAddressRepr(sal_Int32 nColumn, sal_Int32 nRow)
{
    // Adapted from the function lcl_ScColToAlpha() in sc/source/core/tool/address.cxx
    // in LibreOffice/core.git

    OUStringBuffer aBuf;
    if (nColumn < 26 * 26)
    {
        if (nColumn < 26)
            aBuf.append(static_cast<char>('A' + nColumn));
        else
        {
            aBuf.append(static_cast<char>('A' + nColumn / 26 - 1));
            aBuf.append(static_cast<char>('A' + nColumn % 26));
        }
    }
    else
    {
        sal_Int32 nInsert = aBuf.getLength();
        while (nColumn >= 26)
        {
            sal_Int32 nC = nColumn % 26;
            aBuf.insert(nInsert, static_cast<char>('A' + nC));
            nColumn = nColumn - nC;
            nColumn = nColumn / 26 - 1;
        }
        aBuf.insert(nInsert, static_cast<char>('A' + nColumn));
    }

    aBuf.append(nRow + 1, 10);

    return aBuf.makeStringAndClear();
}

OUString getCellRangeRepr(const CellRangeAddress& aRange)
{
    return getCellAddressRepr(aRange.StartColumn, aRange.StartRow) + ":"
           + getCellAddressRepr(aRange.EndColumn, aRange.EndRow);
}

sal_Int32 findMaxInColumn(const Reference<XSpreadsheet>& rxSheet, sal_Int32 nCol,
                          sal_Int32 nStartRow, sal_Int32 nEndRow)
{
    Reference<XCell> xCell;
    sal_Int32 nMax = -1;
    for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
    {
        xCell = rxSheet->getCellByPosition(nCol, nRow);
        double fVal = xCell->getValue();
        sal_Int32 nVal = fVal;
        if (static_cast<double>(nVal) == fVal && nVal > nMax)
            nMax = nVal;
    }

    return nMax;
}
