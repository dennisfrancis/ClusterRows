# ClusterRows
# Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from typing import Tuple

import sys
import inspect
import os

cmd_folder = os.path.realpath(
    os.path.abspath(
        os.path.split(inspect.getfile(inspect.currentframe()))[0]))

if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import crlogger
import crplatform
import crrange
import crcolors

import unohelper
import uno

from com.sun.star.task import XJob
from com.sun.star.awt import XDialogEventHandler, XTextListener, XItemListener
from com.sun.star.sheet import XRangeSelectionListener
from com.sun.star.frame.DispatchResultState import SUCCESS
from com.sun.star.sheet.GeneralFunction import MAX as GeneralFunction_MAX
from com.sun.star.sheet.ConditionOperator import FORMULA as ConditionOperator_FORMULA

MAXROW = 1048575
MAXCOL = 1023

class CRJobImpl(unohelper.Base, XJob):
    def __init__(self, ctx, testMode=False):
        self.ctx = ctx
        self.testMode = testMode
        self.platvars = crplatform.CRPlatForm()
        self.logger = crlogger.setupLogger(self._getLogPath())
        self.logger.debug("INIT CRJobImpl")
        self.logger.debug(self.platvars)
        self.dialog = None
        if not self.testMode:
            self.logger.debug(f'extension path = {self._getExtensionPath()}')

    @staticmethod
    def createInstance(ctx):
        return CRJobImpl(ctx)

    @staticmethod
    def getImplementationName() -> str:
        return "com.github.dennisfrancis.python.CRJobImpl"

    @staticmethod
    def getServiceNames() -> Tuple[str]:
        return ("com.sun.star.task.Job",)

    def _getExtensionURL(self):
        piProvider = self.ctx.getByName("/singletons/com.sun.star.deployment.PackageInformationProvider")
        return piProvider.getPackageLocation('com.github.dennisfrancis.ClusterRowsImpl')

    def _getExtensionPath(self) -> str:
        extension_uri = self._getExtensionURL()
        return unohelper.fileUrlToSystemPath(extension_uri)

    def _getLogPath(self) -> str:
        return os.path.join('build', self.platvars.osName) if self.testMode else self._getExtensionPath()

    def _getSuccessReturn(self):
        if self.envType != "DISPATCH":
            return ()

        res = uno.createUnoStruct("com.sun.star.beans.NamedValue")
        res.Name = "SendDispatchResult"
        resVal = uno.createUnoStruct("com.sun.star.frame.DispatchResultEvent")
        resVal.Source = self
        resVal.State = SUCCESS
        resVal.Result = True
        res.Value = resVal
        return (res, )

    def _parseArgs(self, args):
        for arg in args:
            if arg.Name == "Environment" and len(arg.Value) > 0:
                for envArg in arg.Value:
                    if envArg.Name == "EnvType":
                        self.envType = envArg.Value
                    elif envArg.Name == "EventName":
                        self.eventName = envArg.Value
                    elif envArg.Name == "Frame":
                        self.frame = envArg.Value
                self.controller = self.frame.getController()
                if self.controller is None:
                    return False
                self.model = self.controller.getModel()
                if self.model is None:
                    return False
                return (self.envType in ["EXECUTOR", "DISPATCH"])
        return False

    def _getDataRange(self):
        userSelection = self.model.getCurrentSelection()
        if userSelection is None:
            return False
        # Initial cell range of the data
        self.userRange = crrange.cellRangeToString(userSelection.getRangeAddress(), self.model)
        return True

    def _createDialogAndExecute(self):
        if not self.dialog is None:
            self.dialog.setVisible(True)
            return True

        dialogProvider = self.ctx.getServiceManager() \
            .createInstanceWithContext("com.sun.star.awt.DialogProvider2", self.ctx)
        if dialogProvider is None:
            self.logger.error("CRJobImpl._createDialogAndExecute: cannot create dialogProvider!")
            return False

        xdlFile = self._getExtensionURL() + "/ClusterRows.xdl"
        dlgHandler = CRDialogHandler(self.ctx, self.logger, self.userRange)
        self.dialog = dialogProvider.createDialogWithHandler(xdlFile, dlgHandler)
        if self.dialog is None:
            self.logger.error("CRJobImpl._createDialogAndExecute: cannot create dialog!")
            return False

        dlgHandler.setDialog(self.dialog)
        dlgHandler.setupControlHandlers()
        setDialogRange(self.userRange, self.dialog, self.logger)
        self.dialog.setVisible(True)

        return True

    def _launchClusterDialog(self):
        if not self._getDataRange():
            self.logger.error("CRJobImpl._launchClusterDialog: getDataRange failed!")
            return
        self._createDialogAndExecute()

    def execute(self, args):
        try:
            self._execute(args)
        except Exception as e:
            self.logger.exception("CRJobImpl._execute() crashed.")

    def _execute(self, args):
        self.logger.debug("CRJobImpl.execute: START")
        if not self._parseArgs(args):
            self.logger.debug("CRJobImpl.execute: _parseArgs failed!")
            return ()

        self.logger.debug("CRJobImpl.execute: _parseArgs succeeded")
        self.logger.debug(f'CRJobImpl.execute: envType = {self.envType}, eventName = {self.eventName}, \n\tframe = {self.frame}\n\tmodel = {self.model}')

        if self.eventName == "onClusterRowsReqDialog":
            self._launchClusterDialog()

        return self._getSuccessReturn()

class GMMArgs(object):
    def __init__(self, numClusters = 0, numEpochs = 10, numIterations = 100, colorRows = True, hasHeader = False):
        self.rangeAddr = None
        self.numClusters = numClusters
        self.numEpochs = numEpochs
        self.numIterations = numIterations
        self.colorRows = colorRows
        self.hasHeader = hasHeader

    def drows(self):
        """Returns the number of data rows"""
        rowCount = self.rangeAddr.EndRow - self.rangeAddr.StartRow + 1
        return rowCount - 1 if self.hasHeader else rowCount

    def dcols(self):
        """Returns the number of data columns"""
        return self.rangeAddr.EndColumn - self.rangeAddr.StartColumn + 1


class CRDialogHandler(unohelper.Base, XDialogEventHandler):
    def __init__(self, ctx, logger, userRange):
        self.ctx = ctx
        self.logger = logger
        self.logger.debug("INIT CRDialogHandler")
        self.dialog = None
        self.desktop = self.ctx.ServiceManager.createInstanceWithContext("com.sun.star.frame.Desktop", ctx)
        self.model = self.desktop.getCurrentComponent()
        self.controller = self.model.CurrentController
        self.gmmArgs = GMMArgs()
        self.gmmArgs.rangeAddr = crrange.stringToCellRange(userRange, self.model)

    def setDialog(self, dialog):
        self.dialog = dialog

    def getSupportedMethodNames(self):
        return (
            "onOKButtonPress",
            "onCancelButtonPress",
            "onInputFocusLost",
            "onRangeSelButtonPress")

    def callHandlerMethod(self, dialog, eventObject, methodName):
        self.logger.debug("CRDialogHandler.callHandlerMethod : methodName = " + methodName)
        try:
            if methodName == "onOKButtonPress":
                self.writeResults()
            elif methodName == "onCancelButtonPress":
                dialog.setVisible(False)
            elif methodName == "onInputFocusLost":
                self.validate()
            elif methodName == "onRangeSelButtonPress":
                self._onRangeSelButtonPress(dialog)

        except Exception as e:
            self.logger.exception("CRDialogHandler.callHandlerMethod() crashed.")

        return True

    def showDialog(self):
        if self.dialog is None:
            return

        try:
            if not self.rangeListener.failed:
                self.gmmArgs.rangeAddr = crrange.stringToCellRange(self.rangeListener.rangeStr, self.model)
                setDialogRange(self.rangeListener.rangeStr, self.dialog, self.logger)
                self.validate()

            self.controller.removeRangeSelectionListener(self.rangeListener)
            self.dialog.setVisible(True)
        except Exception as e:
            self.logger.exception("CRDialogHandler.showDialog() crashed")

    def _onRangeSelButtonPress(self, dialog):
        self.dialog = dialog
        self.dialog.setVisible(False)
        self.rangeListener = CRRangeSelectionListener(self)

        self.controller.addRangeSelectionListener(self.rangeListener)
        initialValue = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        title = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        closeOnMouseRelease = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        initialValue.Name = "InitialValue"
        initialValue.Value = crrange.cellRangeToString(self.gmmArgs.rangeAddr, self.model)
        title.Name = "Title"
        title.Value = "Select the cell range where the data is"
        closeOnMouseRelease.Name = "CloseOnMouseRelease"
        closeOnMouseRelease.Value = True

        self.controller.startRangeSelection((initialValue, title, closeOnMouseRelease))

    def readDialogInputs(self):
        rangeStr = self.dialog.getControl("TextField_DataRange").getText()
        self.gmmArgs.rangeAddr = crrange.stringToCellRange(rangeStr, self.model)
        self.gmmArgs.hasHeader = bool(self.dialog.getControl("CheckBox_HasHeader").getState())
        self.gmmArgs.numClusters = int(self.dialog.getControl("NumericField_NumClusters").getValue())
        self.gmmArgs.numEpochs = int(self.dialog.getControl("NumericField_NumEpochs").getValue())
        self.gmmArgs.numIterations = int(self.dialog.getControl("NumericField_NumIter").getValue())
        self.gmmArgs.colorRows = bool(self.dialog.getControl("CheckBox_ColorRows").getState())

    def writeResults(self):
        self.readDialogInputs()
        dataRange = uno.createUnoStruct("com.sun.star.table.CellRangeAddress")
        dataRange.Sheet = self.gmmArgs.rangeAddr.Sheet
        dataRange.StartColumn = self.gmmArgs.rangeAddr.StartColumn
        dataRange.EndColumn = self.gmmArgs.rangeAddr.EndColumn
        dataRange.StartRow = self.gmmArgs.rangeAddr.StartRow + (1 if self.gmmArgs.hasHeader else 0)
        dataRange.EndRow = self.gmmArgs.rangeAddr.EndRow

        resultsRange = uno.createUnoStruct("com.sun.star.table.CellRangeAddress")
        resultsRange.Sheet = dataRange.Sheet
        resultsRange.StartColumn = dataRange.EndColumn + 1
        resultsRange.EndColumn = resultsRange.StartColumn + 1
        resultsRange.StartRow = dataRange.StartRow
        resultsRange.EndRow = dataRange.EndRow

        undoMgr = self.model.getUndoManager()
        if not undoMgr is None:
            undoMgr.enterUndoContext("ClusterRowsImpl_UNDO")

        resRangeObj = crrange.rangeAddressToObject(resultsRange, self.model)
        formulaName = "COM.GITHUB.DENNISFRANCIS.PYTHON.GMMCLUSTERIMPL.GMMCLUSTER"
        rangeArg = crrange.cellRangeToString(dataRange, self.model)
        args = self.gmmArgs
        resRangeObj.setArrayFormula(
            f"={formulaName}({rangeArg};{args.numClusters};{args.numEpochs};{args.numIterations})")

        if args.hasHeader:
            sheet = self.model.Sheets[dataRange.Sheet]
            cell = sheet.getCellByPosition(resultsRange.StartColumn, resultsRange.StartRow - 1)
            cell.setFormula("ClusterId")
            cell = sheet.getCellByPosition(resultsRange.StartColumn + 1, resultsRange.StartRow - 1)
            cell.setFormula("Confidence")

        # Find actual number of clusters if in auto mode
        self._updateNumClusters(resRangeObj)

        if self.gmmArgs.colorRows:
            self._addClusterStyles()
            self._colorClusterData(dataRange)

        if not undoMgr is None:
            undoMgr.leaveUndoContext()

        self.dialog.setVisible(False)
        return

    def _updateNumClusters(self, resRangeObj):
        if self.gmmArgs.numClusters > 0:
            return

        maxClusterId = resRangeObj.computeFunction(GeneralFunction_MAX)
        if maxClusterId != -1:
            self.gmmArgs.numClusters = int(maxClusterId + 1)

    def _addClusterStyles(self):
        numClusters = self.gmmArgs.numClusters
        styleFamilies = self.model.getStyleFamilies()
        cellStyles = styleFamilies.getByName("CellStyles")
        colors = crcolors.getClusterColors(numClusters)
        for idx in range(numClusters):
            color = colors[idx]
            styleName = self._getStyleName(idx, numClusters)
            clusterStyle = None
            if cellStyles.hasByName(styleName):
                clusterStyle = cellStyles.getByName(styleName)
            else:
                clusterStyle = self.model.createInstance("com.sun.star.style.CellStyle")
                cellStyles.insertByName(styleName, clusterStyle)

            clusterStyle.setPropertyValue("CellBackColor", color)
            self.logger.debug(f"CRDialogHandler._addClusterStyles: added cell style : {styleName} with backcolor = {hex(color)}")

    def _colorClusterData(self, dataRange):
        rangeObj = crrange.rangeAddressToObject(dataRange, self.model)
        cfEntries = rangeObj.getPropertyValue("ConditionalFormat")
        if cfEntries is None:
            self.logger.error("CRDialogHandler._colorClusterData: cannot get XSheetConditionalEntries from data range")
            return
        cfEntries.clear()

        # row is 0 which is relative w.r.t data table's top row
        refCell = "$" + crrange.cellAddressToString(dataRange.EndColumn + 1, 0, absolute=False)
        self.logger.debug(f"CRDialogHandler._colorClusterData: refCell = {refCell}")

        numClusters = self.gmmArgs.numClusters
        for idx in range(numClusters):
            operator = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
            operator.Name = "Operator"
            operator.Value = ConditionOperator_FORMULA

            formula1 = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
            formula1.Name = "Formula1"
            formula1.Value = f"{refCell}={idx}"

            styleName = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
            styleName.Name = "StyleName"
            styleName.Value = self._getStyleName(idx, numClusters)

            cfEntries.addNew((operator, formula1, styleName))

        rangeObj.setPropertyValue("ConditionalFormat", cfEntries)

    def _getStyleName(self, clusterIndex, numClusters):
        return f"ClusterRows_N{numClusters}_Cluster_{clusterIndex}"

    def _markFieldError(self, controlName, hasError = True, defColor = None):
        model = self.dialog.getControl(controlName).getModel()
        model.setPropertyValue("TextColor", 0xff0000 if hasError else None)

    def validate(self):
        try:
            self._validate()
        except Exception as e:
            self.logger.exception("CRDialogHandler.validate() crashed.")

    def _validate(self):
        self.readDialogInputs()

        if self.gmmArgs.rangeAddr is None:
            self._setStatus("Invalid data range!")
            self._markFieldError("TextField_DataRange")
            return

        # Not enough rows
        if self.gmmArgs.drows() < 10:
            self._setStatus("There must be at least 10 samples")
            self._markFieldError("TextField_DataRange")
            return

        # No space to write results
        if self.gmmArgs.rangeAddr.EndColumn > MAXCOL - 2:
            self._setStatus("Not enough space to write the results (2 columns)")
            self._markFieldError("TextField_DataRange")
            return

        self._markFieldError("TextField_DataRange", hasError=False)
        # Range is valid, preserve it by creating a selection.
        selectRange(self.gmmArgs.rangeAddr, self.model, self.logger)

        # numCluster checks
        if self.gmmArgs.numClusters < 0 or self.gmmArgs.numClusters > 15:
            self._setStatus("Number of clusters must be in the range [0, 15]")
            self._markFieldError("NumericField_NumClusters")
            return

        self._markFieldError("NumericField_NumClusters", hasError=False)

        # numEpoch checks
        if self.gmmArgs.numEpochs < 3 or self.gmmArgs.numEpochs > 100:
            self._setStatus("Number of epochs expected to be in the range [3, 100]")
            self._markFieldError("NumericField_NumEpochs")
            return

        self._markFieldError("NumericField_NumEpochs", hasError=False)

        # numIterations checks
        if self.gmmArgs.numIterations < 5 or self.gmmArgs.numIterations > 10000:
            self._setStatus("Number of iterations expected to be in the range [5, 10000]")
            self._markFieldError("NumericField_NumIter")
            return

        self._markFieldError("NumericField_NumIter", hasError=False)

        # All OK.
        self._setStatus()

    def _setStatus(self, errMsg: str = ""):
        hasError = bool(errMsg)
        computeButton = self.dialog.getControl("CommandButton_OK")
        errorLabel = self.dialog.getControl("LabelText_Error")
        computeButton.getModel().setPropertyValue("Enabled", not hasError)
        errorLabel.setText(("Error: " + errMsg) if hasError else errMsg)

    def setupControlHandlers(self):
        self.dataRangeTextListener = CRDataRangeTextListener(self)
        self.dialog.getControl("TextField_DataRange").addTextListener(self.dataRangeTextListener)
        self.headerCheckBoxListener = CRHeaderCheckBoxListener(self)
        self.dialog.getControl("CheckBox_HasHeader").addItemListener(self.headerCheckBoxListener)

    def clearDialogControlHandlers(self):
        self.dialog.getControl("TextField_DataRange").removeTextListener(self.dataRangeTextListener)
        self.dialog.getControl("CheckBox_HasHeader").removeItemListener(self.headerCheckBoxListener)

# global functions

def setDialogRange(rangeStr, dialog, logger):
    if not isinstance(rangeStr, str):
        rangeStr = "NONE"
    label = dialog.getControl("TextField_DataRange")
    if label is None:
        logger.error("global.setDialogRange: cannot get DataRange label control from dialog")
        return False
    label.setText(rangeStr)
    return True

def selectRange(rangeAddress, document, logger):
    rangeObj = crrange.rangeAddressToObject(rangeAddress, document)
    if rangeObj is None:
        logger.error('global.selectRange: crrange.stringToRangeObj returned None!')
        return
    document.getCurrentController().select(rangeObj)


class CRRangeSelectionListener(unohelper.Base, XRangeSelectionListener):
    def __init__(self, dlgHandler):
        self.dlgHandler = dlgHandler
        self.failed = False
        self.rangeStr = ""
        return

    def done(self, event):
        self.rangeStr = event.RangeDescriptor
        self.failed = False
        self.dlgHandler.showDialog()

    def aborted(self, event):
        self.failed = True
        self.dlgHandler.showDialog()

    def disposing(self, event):
        return


class CRDataRangeTextListener(unohelper.Base, XTextListener):
    def __init__(self, dlgHandler):
        self.dlgHandler = dlgHandler

    def textChanged(self, e):
        self.dlgHandler.validate()

    def disposing(self, e):
        return


class CRHeaderCheckBoxListener(unohelper.Base, XItemListener):
    def __init__(self, dlgHandler):
        self.dlgHandler = dlgHandler

    def itemStateChanged(self, e):
        self.dlgHandler.validate()

    def disposing(self, e):
        return
