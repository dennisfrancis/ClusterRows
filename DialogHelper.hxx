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

#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/awt/XDialogEventHandler.hpp>

#include <functional>

#include "params.hxx"

namespace com::sun::star
{
namespace uno
{
class XComponentContext;
}

namespace awt
{
class XDialog;
}

}

namespace dialoghelper
{
::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog> createDialog(
    const ::rtl::OUString& aDialogXDL,
    const ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext>& xContext,
    const ::com::sun::star::uno::Reference<::com::sun::star::awt::XDialogEventHandler>& xHandler,
    const rtl::OUString& aCellRangeRepr);

bool onAction(const ::rtl::OUString& actionName,
              const ::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog>& xDialog,
              const std::function<void(const ClusterParams&)>& rClusterCallback);

void show(::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog> xDialog);

}
