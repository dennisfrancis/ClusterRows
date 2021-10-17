#include <stdio.h>
#include <rtl/ustring.hxx>
#include <uno/lbnames.h>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/factory.hxx>
// generated c++ interfaces
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/registry/XRegistryKey.hpp>

// include our specific addon header to get access to functions and definitions
#include "cluster.hxx"
#include "GMMCluster.hxx"

using namespace ::rtl;
using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;

/**
 * This function is called to get service factories for an implementation.
 *
 * @param pImplName       name of implementation
 * @param pServiceManager a service manager, need for component creation
 * @param pRegistryKey    the registry key for this component, need for persistent data
 * @return a component factory
 */
extern "C" SAL_DLLPUBLIC_EXPORT void *SAL_CALL component_getFactory(const sal_Char *pImplName, void * /*pServiceManager*/, void *pRegistryKey)
{
    void *pRet = 0;

    if (rtl_str_compare(pImplName, IMPLEMENTATION_NAME) == 0)
    {
        Reference<XSingleComponentFactory> xFactory(createSingleComponentFactory(
            ClusterRowsImpl_createInstance,
            OUString(IMPLEMENTATION_NAME),
            ClusterRowsImpl_getSupportedServiceNames()));

        if (xFactory.is())
        {
            xFactory->acquire();
            pRet = xFactory.get();
        }
    }
    else if (rtl_str_compare(pImplName, ADDIN_IMPLEMENTATION_NAME) == 0)
    {
        Reference<XSingleComponentFactory> xFactory(createSingleComponentFactory(
            GMMClusterImpl_createInstance,
            OUString(ADDIN_IMPLEMENTATION_NAME),
            GMMClusterImpl_getSupportedServiceNames()));

        if (xFactory.is())
        {
            xFactory->acquire();
            pRet = xFactory.get();
        }
    }

    return pRet;
}

extern "C" SAL_DLLPUBLIC_EXPORT void SAL_CALL
component_getImplementationEnvironment(
    char const **ppEnvTypeName, uno_Environment **)
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}
