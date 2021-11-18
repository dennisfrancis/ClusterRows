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

#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.h>

namespace com::sun::star
{
namespace uno
{
class XComponentContext;
}

namespace frame
{
class XFrame;
class XModel;
}

namespace sheet
{
class XSpreadsheet;
}

namespace table
{
struct CellRangeAddress;
}

}

namespace helper
{
com::sun::star::uno::Reference<com::sun::star::frame::XModel>
getModel(const com::sun::star::uno::Reference<com::sun::star::frame::XFrame>& rxFrame);

bool getDataRange(const com::sun::star::uno::Reference<com::sun::star::frame::XModel>& rxModel,
                  com::sun::star::table::CellRangeAddress& rRangeExtended);

com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>
getSheet(const com::sun::star::uno::Reference<com::sun::star::frame::XModel>& rxModel,
         const sal_Int32 nSheet);

void showErrorMessage(
    const com::sun::star::uno::Reference<com::sun::star::frame::XFrame>& xFrame,
    const rtl::OUString& aTitle, const rtl::OUString& aMsgText,
    const com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext>& xCtxt);

rtl::OUString getStyleName(sal_Int32 nClusterIdx, sal_Int32 nNumClusters);
}