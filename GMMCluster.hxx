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

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/github/dennisfrancis/XGMMCluster.hpp>
#include <com/sun/star/sheet/XAddIn.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XServiceName.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <cppuhelper/implbase4.hxx>

#define ADDIN_IMPLEMENTATION_NAME "com.github.dennisfrancis.GMMClusterImpl"
#define NUMFUNCTIONS 1
#define NUMARGS 4

class GMMClusterImpl : public cppu::WeakImplHelper4<
                           com::github::dennisfrancis::XGMMCluster, com::sun::star::sheet::XAddIn,
                           com::sun::star::lang::XServiceName, com::sun::star::lang::XServiceInfo>
{
private:
    static const sal_Int32 nNumFunctions = NUMFUNCTIONS;
    static const ::rtl::OUString aFunctionNames[NUMFUNCTIONS];
    static const ::rtl::OUString aDisplayFunctionNames[NUMFUNCTIONS];
    static const ::rtl::OUString aDescriptions[NUMFUNCTIONS];
    static const ::rtl::OUString aArgumentNames[NUMFUNCTIONS][NUMARGS];
    static const ::rtl::OUString aArgumentDescriptions[NUMFUNCTIONS][NUMARGS];

    com::sun::star::lang::Locale aFuncLocale;

public:
    GMMClusterImpl() {}

    // XGMMCluster
    virtual ::com::sun::star::uno::Sequence<::com::sun::star::uno::Sequence<double>>
        SAL_CALL gmmCluster(const ::com::sun::star::uno::Sequence<
                                ::com::sun::star::uno::Sequence<::com::sun::star::uno::Any>>& data,
                            const ::com::sun::star::uno::Any& numClusters,
                            const ::com::sun::star::uno::Any& numEpochs,
                            const ::com::sun::star::uno::Any& numIterations);

    // XAddIn
    rtl::OUString getProgrammaticFuntionName(const rtl::OUString& aDisplayName);
    rtl::OUString getDisplayFunctionName(const rtl::OUString& aProgrammaticName);
    rtl::OUString getFunctionDescription(const rtl::OUString& aProgrammaticName);
    rtl::OUString getDisplayArgumentName(const rtl::OUString& aProgrammaticFunctionName,
                                         sal_Int32 nArgument);
    rtl::OUString getArgumentDescription(const rtl::OUString& aProgrammaticFunctionName,
                                         sal_Int32 nArgument);
    rtl::OUString getProgrammaticCategoryName(const rtl::OUString& aProgrammaticFunctionName);
    rtl::OUString getDisplayCategoryName(const rtl::OUString& aProgrammaticFunctionName);

    //  XLocalizable
    void setLocale(const com::sun::star::lang::Locale& aLocale) { aFuncLocale = aLocale; }
    com::sun::star::lang::Locale getLocale() { return aFuncLocale; }

    // XServiceInfo methods
    virtual ::rtl::OUString SAL_CALL getImplementationName();
    virtual sal_Bool SAL_CALL supportsService(const ::rtl::OUString& aServiceName);
    virtual ::com::sun::star::uno::Sequence<::rtl::OUString> SAL_CALL getSupportedServiceNames();

    ::rtl::OUString getServiceName();

private:
    sal_Int32 getFunctionID(const ::rtl::OUString aProgrammaticFunctionName) const;
};

::rtl::OUString GMMClusterImpl_getImplementationName();

sal_Bool SAL_CALL GMMClusterImpl_supportsService(const ::rtl::OUString& ServiceName);

::com::sun::star::uno::Sequence<::rtl::OUString> SAL_CALL GMMClusterImpl_getSupportedServiceNames();

::com::sun::star::uno::Reference<::com::sun::star::uno::XInterface>
    SAL_CALL GMMClusterImpl_createInstance(
        const ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext>& rContext);
