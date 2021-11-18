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

#define MAXROW 1048575
#define MAXCOL 1023

#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.h>

namespace com::sun::star
{
namespace sheet
{
class XSpreadsheet;
}

namespace table
{
struct CellRangeAddress;
}

}

namespace range
{
bool rangeIsSingleCell(const com::sun::star::table::CellRangeAddress& rRange);

bool isCellEmpty(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
                 sal_Int32 nCol, sal_Int32 nRow);

bool isColumnEmpty(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
    sal_Int32 nCol, sal_Int32 nStartRow, sal_Int32 nEndRow);

bool isRowEmpty(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
                sal_Int32 nRow, sal_Int32 nStartCol, sal_Int32 nEndCol);

void shrinkRangeToData(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
    com::sun::star::table::CellRangeAddress& rRangeExtended);

void expandRangeToData(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
    com::sun::star::table::CellRangeAddress& rRangeExtended);

void excludeResultColumns(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxSheet,
    com::sun::star::table::CellRangeAddress& rRangeExtended);

bool hasHeader(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& xSheet,
               com::sun::star::table::CellRangeAddress& aRange);

rtl::OUString getCellAddressRepr(sal_Int32 nColumn, sal_Int32 nRow);

rtl::OUString getCellRangeRepr(const com::sun::star::table::CellRangeAddress& aRange);

sal_Int32
findMaxInColumn(const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& xSheet,
                sal_Int32 nCol, sal_Int32 nStartRow, sal_Int32 nEndRow);

}
