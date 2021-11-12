#ifndef COM_GITHUB_DENNISFRANCIS_CLUSTERROWS_HXX
#define COM_GITHUB_DENNISFRANCIS_CLUSTERROWS_HXX

#include <com/sun/star/task/XJob.hpp>
#include <com/sun/star/awt/XDialogEventHandler.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <cppuhelper/implbase3.hxx>

#include "params.hxx"

#define IMPLEMENTATION_NAME "com.github.dennisfrancis.ClusterRowsImpl"

namespace com::sun::star
{
namespace frame
{
class XFrame;
}

namespace uno
{
class XComponentContext;
}

namespace beans
{
struct NamedValue;
}

namespace table
{
struct CellRangeAddress;
}

namespace sheet
{
struct XSpreadsheet;
}

}

class ClusterRowsImpl : public cppu::WeakImplHelper3<com::sun::star::task::XJob,
                                                     com::sun::star::awt::XDialogEventHandler,
                                                     com::sun::star::lang::XServiceInfo>
{
private:
    ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext> mxContext;
    ::com::sun::star::uno::Reference<::com::sun::star::sheet::XSpreadsheet> mxSheet;
    ClusterParams maParams;
    ::com::sun::star::table::CellRangeAddress maDataRange;
    bool mbHasHeader : 1;

public:
    ClusterRowsImpl(
        const ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext>&
            rxContext);
    ~ClusterRowsImpl();

    // XAsyncJob methods
    virtual ::com::sun::star::uno::Any SAL_CALL execute(
        const ::com::sun::star::uno::Sequence<::com::sun::star::beans::NamedValue>& rArgs) override;

    // XDialogEventHandler methods
    virtual sal_Bool callHandlerMethod(
        const ::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog>& xDialog,
        const ::com::sun::star::uno::Any& eventObject, const ::rtl::OUString& methodName) override;
    virtual ::com::sun::star::uno::Sequence<::rtl::OUString> getSupportedMethodNames() override;

    // XServiceInfo methods
    virtual ::rtl::OUString SAL_CALL getImplementationName();
    virtual sal_Bool SAL_CALL supportsService(const ::rtl::OUString& aServiceName);
    virtual ::com::sun::star::uno::Sequence<::rtl::OUString> SAL_CALL getSupportedServiceNames();

    // A struct to store some job related info when execute() is called
    struct ClusterRowsImplInfo
    {
        ::rtl::OUString aEnvType;
        ::rtl::OUString aEventName;
        ::rtl::OUString aAlias;
        ::com::sun::star::uno::Reference<::com::sun::star::frame::XFrame> xFrame;
    };

private:
    ::rtl::OUString validateGetInfo(
        const ::com::sun::star::uno::Sequence<::com::sun::star::beans::NamedValue>& rArgs,
        ClusterRowsImplInfo& rJobInfo);

    void clusterRows(const ClusterRowsImplInfo& rJobInfo, const sal_Int32 nUserNumClusters);
    bool calcDataRange(const ClusterRowsImplInfo& rJobInfo,
                       ::com::sun::star::table::CellRangeAddress& aRange) const;
    void launchClusterDialog(const ClusterRowsImplInfo& aJobInfo);
    void writeResults() const;
};

::rtl::OUString ClusterRowsImpl_getImplementationName();

sal_Bool SAL_CALL ClusterRowsImpl_supportsService(const ::rtl::OUString& ServiceName);

::com::sun::star::uno::Sequence<::rtl::OUString>
    SAL_CALL ClusterRowsImpl_getSupportedServiceNames();

::com::sun::star::uno::Reference<::com::sun::star::uno::XInterface>
    SAL_CALL ClusterRowsImpl_createInstance(
        const ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext>& rContext);

#endif
