#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/awt/XDialogEventHandler.hpp>

#include <functional>

#include "params.hxx"

namespace com::sun::star {
    namespace uno {
        class XComponentContext;
    }

    namespace awt {
        class XDialog;
    }
}

namespace dialoghelper {

::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog>
createDialog(
    const ::rtl::OUString& aDialogXDL,
    const ::com::sun::star::uno::Reference<::com::sun::star::uno::XComponentContext>& xContext,
    const ::com::sun::star::uno::Reference<::com::sun::star::awt::XDialogEventHandler>& xHandler,
    const rtl::OUString& aCellRangeRepr);

bool onAction(
    const ::rtl::OUString& actionName,
    const ::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog>& xDialog,
    const std::function<void(const ClusterParams&)>& rClusterCallback);

void show(::com::sun::star::uno::Reference<::com::sun::star::awt::XDialog> xDialog);

}
