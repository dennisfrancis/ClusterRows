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

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XFixedText.hpp>
#include <com/sun/star/awt/XCheckBox.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XNumericField.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/deployment/XPackageInformationProvider.hpp>
#include <com/sun/star/deployment/PackageInformationProvider.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <osl/file.hxx>

#include "logging.hxx"
#include "DialogHelper.hxx"

using namespace com::sun::star::deployment;
using namespace com::sun::star::awt;
using namespace com::sun::star::beans;
using namespace com::sun::star::uno;
using namespace rtl;

static OUString convertToURL(const OUString& aFileInRoot,
                             const Reference<::com::sun::star::uno::XComponentContext>& xContext);

static void validateInputs(const Reference<XDialog>& xDialog);
static Reference<XControl> getControl(const OUString& aName, const Reference<XDialog>& xDialog);
static Reference<XNumericField> getNumericField(const OUString& aName,
                                                const Reference<XDialog>& xDialog);
static Reference<XCheckBox> getCheckBox(const OUString& aName, const Reference<XDialog>& xDialog);
static ClusterParams getParamsFromDialog(const Reference<XDialog>& xDialog);
static void setError(const OUString& aMsg, const Reference<XDialog>& xDialog);
static void setMessage(const OUString& aMsg, const Reference<XDialog>& xDialog);
static void resetError(const Reference<XDialog>& xDialog);
static void setControlStatus(const OUString& aName, bool bEnabledStatus,
                             const Reference<XDialog>& xDialog);

bool dialoghelper::onAction(const OUString& actionName, const Reference<XDialog>& xDialog,
                            const std::function<void(const ClusterParams&)>& rClusterCallback)
{
    writeLog("onAction: actionName = %s\n", actionName.toUtf8().getStr());
    if (actionName == "onOKButtonPress")
    {
        ClusterParams aParams = getParamsFromDialog(xDialog);
        writeLog("onOKButtonPress: aParams: mnNumClusters = %d, mnNumEpochs = %d, mnNumIterations "
                 "= %d mbColorClusters = %s\n",
                 aParams.mnNumClusters, aParams.mnNumEpochs, aParams.mnNumIterations,
                 aParams.mbColorClusters ? "true" : "false");
        setMessage("Computing clusters...", xDialog);
        rClusterCallback(aParams);
        xDialog->endExecute();
        return true;
    }
    else if (actionName == "onCancelButtonPress")
    {
        xDialog->endExecute();
        return true;
    }
    else if (actionName == "onInputFocusLost")
    {
        validateInputs(xDialog);
        return true;
    }

    return false;
}

Reference<XDialog> dialoghelper::createDialog(const OUString& aDialogXDL,
                                              const Reference<XComponentContext>& xContext,
                                              const Reference<XDialogEventHandler>& xHandler,
                                              const OUString& aCellRangeRepr)
{
    Reference<XDialogProvider2> xDialogProvider(
        xContext->getServiceManager()->createInstanceWithContext("com.sun.star.awt.DialogProvider2",
                                                                 xContext),
        UNO_QUERY);

    if (!xDialogProvider.is())
        return Reference<XDialog>();

    Reference<XDialog> xDialog
        = xDialogProvider->createDialogWithHandler(convertToURL(aDialogXDL, xContext), xHandler);
    if (xDialog.is())
    {
        Reference<XFixedText> xLabel(getControl("LabelField_DataRange", xDialog), UNO_QUERY);
        if (!xLabel.is())
        {
            writeLog("createDialog: FAILED: cannot get XFixedText from XControl!\n");
            return xDialog;
        }
        xLabel->setText(aCellRangeRepr);
    }

    return xDialog;
}

static OUString convertToURL(const OUString& aFileInRoot,
                             const Reference<::com::sun::star::uno::XComponentContext>& xContext)
{
    Reference<XPackageInformationProvider> xInfoProvider(PackageInformationProvider::get(xContext));
    OUString aURL = xInfoProvider->getPackageLocation("com.github.dennisfrancis.ClusterRowsImpl")
                    + "/" + aFileInRoot;
    osl::File aFile(aURL);
    osl::FileBase::RC aRet = aFile.open(osl_File_OpenFlag_Read);
    if (aRet != osl::FileBase::E_None)
    {
        writeLog("convertToURL file open FAILED: aFileInRoot = %s, aURL = %s\n",
                 aFileInRoot.toUtf8().getStr(), aURL.toUtf8().getStr());
        return "";
    }
    aRet = aFile.close();
    if (aRet != osl::FileBase::E_None)
    {
        writeLog("convertToURL file close FAILED: aFileInRoot = %s, aURL = %s\n",
                 aFileInRoot.toUtf8().getStr(), aURL.toUtf8().getStr());
        return "";
    }
    return aURL;
}

static void validateInputs(const Reference<XDialog>& xDialog)
{
    ClusterParams aParams = getParamsFromDialog(xDialog);
    if (aParams.mnNumClusters < 0 || aParams.mnNumClusters > 15)
    {
        setError("Number of clusters must be in the range [0, 15]", xDialog);
        return;
    }

    if (aParams.mnNumEpochs < 3 || aParams.mnNumEpochs > 100)
    {
        setError("Number of clusters must be in the range [3, 100]", xDialog);
        return;
    }

    if (aParams.mnNumIterations < 5 || aParams.mnNumIterations > 10000)
    {
        setError("Number of clusters must be in the range [3, 100]", xDialog);
        return;
    }

    resetError(xDialog);
}

static void setError(const OUString& aMsg, const Reference<XDialog>& xDialog)
{
    setMessage(aMsg, xDialog);

    setControlStatus("CommandButton_OK", aMsg.isEmpty(), xDialog);
}

static void setMessage(const OUString& aMsg, const Reference<XDialog>& xDialog)
{
    Reference<XControl> xCtrl(getControl("LabelText_Error", xDialog));
    if (!xCtrl.is())
    {
        writeLog("setMessage: FAILED: cannot get error label control!\n");
        return;
    }

    Reference<XFixedText> xLabel(xCtrl, UNO_QUERY);
    if (!xLabel.is())
    {
        writeLog("setMessage: FAILED: cannot get XFixedText from XControl!\n");
        return;
    }

    xLabel->setText(aMsg);
}

static void resetError(const Reference<XDialog>& xDialog) { setError("", xDialog); }

static void setControlStatus(const OUString& aName, bool bEnabledStatus,
                             const Reference<XDialog>& xDialog)
{
    Reference<XControl> xCtrl = getControl(aName, xDialog);
    if (!xCtrl.is())
    {
        writeLog("setError: FAILED: cannot get control name = %s!\n", aName.toUtf8().getStr());
        return;
    }

    Reference<XPropertySet> xCtrlProps(xCtrl->getModel(), UNO_QUERY);
    if (!xCtrlProps.is())
    {
        writeLog("setError: FAILED: cannot get control props name = %s!\n",
                 aName.toUtf8().getStr());
        return;
    }

    xCtrlProps->setPropertyValue("Enabled", Any(bEnabledStatus));
}

static ClusterParams getParamsFromDialog(const Reference<XDialog>& xDialog)
{
    ClusterParams aParams{ -1, -1, -1, false };
    if (Reference<XNumericField> xField = getNumericField("NumericField_NumClusters", xDialog);
        xField.is())
    {
        aParams.mnNumClusters = xField->getValue();
    }

    if (Reference<XNumericField> xField = getNumericField("NumericField_NumEpochs", xDialog);
        xField.is())
    {
        aParams.mnNumEpochs = xField->getValue();
    }

    if (Reference<XNumericField> xField = getNumericField("NumericField_NumIter", xDialog);
        xField.is())
    {
        aParams.mnNumIterations = xField->getValue();
    }

    if (Reference<XCheckBox> xField = getCheckBox("CheckBox_ColorRows", xDialog); xField.is())
    {
        aParams.mbColorClusters = static_cast<bool>(xField->getState());
    }

    return aParams;
}

static Reference<XNumericField> getNumericField(const OUString& aName,
                                                const Reference<XDialog>& xDialog)
{
    Reference<XControl> xCtrl(getControl(aName, xDialog));
    if (!xCtrl.is())
    {
        writeLog("getNumericField FAILED: getControl returned no XControl for aName = %s\n",
                 aName.toUtf8().getStr());
        return Reference<XNumericField>();
    }

    Reference<XNumericField> xNumField(xCtrl, UNO_QUERY);
    if (!xNumField.is())
        writeLog(
            "getNumericField FAILED: could not get XNumericField from XControl for aName = %s\n",
            aName.toUtf8().getStr());

    return xNumField;
}

static Reference<XCheckBox> getCheckBox(const OUString& aName, const Reference<XDialog>& xDialog)
{
    Reference<XControl> xCtrl(getControl(aName, xDialog));
    if (!xCtrl.is())
    {
        writeLog("getCheckBox FAILED: getControl returned no XControl for aName = %s\n",
                 aName.toUtf8().getStr());
        return Reference<XCheckBox>();
    }

    Reference<XCheckBox> xCheck(xCtrl, UNO_QUERY);
    if (!xCheck.is())
        writeLog("getCheckBox FAILED: could not get XCheckBox from XControl for aName = %s\n",
                 aName.toUtf8().getStr());

    return xCheck;
}

static Reference<XControl> getControl(const OUString& aName, const Reference<XDialog>& xDialog)
{
    Reference<XControlContainer> xDlgContainer(xDialog, UNO_QUERY);
    if (!xDlgContainer.is())
    {
        writeLog("getControl FAILED: cannot obtain XControlContainer from XDialog!\n");
        return Reference<XControl>();
    }

    return xDlgContainer->getControl(aName);
}
