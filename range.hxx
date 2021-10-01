#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/table/CellContentType.hpp>
#include <vector>
#include "datatypes.hxx"

#define MAXROW 1048575
#define MAXCOL 1023

using com::sun::star::sheet::XSpreadsheet;
using com::sun::star::table::CellContentType_EMPTY;
using com::sun::star::table::CellRangeAddress;
using com::sun::star::table::XCell;
using com::sun::star::uno::Reference;

void shrinkRangeToData(const Reference<XSpreadsheet> &rxSheet, CellRangeAddress &rRangeExtended)
{
	sal_Int32 nStartCol = rRangeExtended.StartColumn;
	sal_Int32 nEndCol = rRangeExtended.EndColumn;
	sal_Int32 nStartRow = rRangeExtended.StartRow;
	sal_Int32 nEndRow = rRangeExtended.EndRow;

	bool bStop = false;
	// Shrink nStartCol
	for (sal_Int32 nCol = nStartCol; (nCol <= nEndCol && !bStop); ++nCol)
	{
		bool bColEmpty = true;
		for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bColEmpty = false;
				break;
			}
		}
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
		bool bColEmpty = true;
		for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bColEmpty = false;
				break;
			}
		}
		if (!bColEmpty)
		{
			bStop = true;
			nEndCol = nCol;
		}
		else if (nCol == nStartCol)
			nEndCol = nStartCol;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);

	bStop = false;
	// Shrink nStartRow
	for (sal_Int32 nRow = nStartRow; (nRow <= nEndRow && !bStop); ++nRow)
	{
		bool bRowEmpty = true;
		for (sal_Int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				//printf("DEBUG>>> found cell at col = %d, row = %d non-empty\n", nCol, nRow ); fflush(stdout);
				bRowEmpty = false;
				break;
			}
		}
		if (!bRowEmpty)
		{
			bStop = true;
			nStartRow = nRow;
		}
		else if (nRow == nEndRow)
			nStartRow = nEndRow;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);
	//printf("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow); fflush(stdout);

	bStop = false;
	// Shrink nEndRow
	for (sal_Int32 nRow = nEndRow; (nRow >= nStartRow && !bStop); --nRow)
	{
		bool bRowEmpty = true;
		for (sal_Int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bRowEmpty = false;
				break;
			}
		}
		if (!bRowEmpty)
		{
			bStop = true;
			nEndRow = nRow;
		}
		else if (nRow == nStartRow)
			nEndRow = nStartRow;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);
	//printf("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow); fflush(stdout);

	rRangeExtended.StartRow = nStartRow;
	rRangeExtended.EndRow = nEndRow;
	rRangeExtended.StartColumn = nStartCol;
	rRangeExtended.EndColumn = nEndCol;
}

void expandRangeToData(const Reference<XSpreadsheet> &rxSheet, CellRangeAddress &rRangeExtended)
{
	sal_Int32 nStartCol = rRangeExtended.StartColumn;
	sal_Int32 nEndCol = rRangeExtended.EndColumn;
	sal_Int32 nStartRow = rRangeExtended.StartRow;
	sal_Int32 nEndRow = rRangeExtended.EndRow;

	bool bStop = false;
	// Extend nStartCol
	for (sal_Int32 nCol = nStartCol - 1; (nCol >= 0 && !bStop); --nCol)
	{
		bool bColEmpty = true;
		for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bColEmpty = false;
				break;
			}
		}
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
		bool bColEmpty = true;
		for (sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bColEmpty = false;
				break;
			}
		}
		if (bColEmpty)
		{
			bStop = true;
			nEndCol = nCol - 1;
		}
		else if (nCol == MAXCOL)
			nEndCol = MAXCOL;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);

	bStop = false;
	// Extend nStartRow
	for (sal_Int32 nRow = nStartRow - 1; (nRow >= 0 && !bStop); --nRow)
	{
		bool bRowEmpty = true;
		for (sal_Int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				//printf("DEBUG>>> found cell at col = %d, row = %d non-empty\n", nCol, nRow ); fflush(stdout);
				bRowEmpty = false;
				break;
			}
		}
		if (bRowEmpty)
		{
			bStop = true;
			nStartRow = nRow + 1;
		}
		else if (nRow == 0)
			nStartRow = 0;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);
	//printf("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow); fflush(stdout);

	bStop = false;
	// Extend nEndRow
	for (sal_Int32 nRow = nEndRow + 1; (nRow <= MAXROW && !bStop); ++nRow)
	{
		bool bRowEmpty = true;
		for (sal_Int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
		{
			Reference<XCell> xCell = rxSheet->getCellByPosition(nCol, nRow);
			if (!xCell.is())
			{
				printf("DEBUG>>> getDataRange : xCell(%d, %d) is invalid.\n", nCol, nRow);
				fflush(stdout);
			}
			else if (xCell->getType() != CellContentType_EMPTY)
			{
				bRowEmpty = false;
				break;
			}
		}
		if (bRowEmpty)
		{
			bStop = true;
			nEndRow = nRow - 1;
		}
		else if (nRow == MAXROW)
			nEndRow = MAXROW;
	}

	//printf("DEBUG>>> nStartCol = %d, nEndCol = %d\n", nStartCol, nEndCol); fflush(stdout);
	//printf("DEBUG>>> nStartRow = %d, nEndRow = %d\n", nStartRow, nEndRow); fflush(stdout);

	rRangeExtended.StartRow = nStartRow;
	rRangeExtended.EndRow = nEndRow;
	rRangeExtended.StartColumn = nStartCol;
	rRangeExtended.EndColumn = nEndCol;
}

void excludeResultColumns(const Reference<XSpreadsheet> &rxSheet, CellRangeAddress &rRangeExtended)
{
	// Result headers in reverse order ( with rightmost header first )
	std::vector<OUString> aResultHeaders = {"Confidence", "ClusterId"};
	Reference<XCell> xCell;
	for (OUString &aHdr : aResultHeaders)
	{
		xCell = rxSheet->getCellByPosition(rRangeExtended.EndColumn, rRangeExtended.StartRow);
		if (!xCell.is())
		{
			printf("DEBUG>>> excludeResultColumns : xCell(%d, %d) is invalid.\n", rRangeExtended.EndColumn, rRangeExtended.StartRow);
			fflush(stdout);
			return;
		}
		if (xCell->getFormula() == aHdr)
			--rRangeExtended.EndColumn;
	}
}
