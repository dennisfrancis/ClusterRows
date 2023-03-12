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

from typing import Tuple, Any

import sys
import inspect
import os

cur_frame = inspect.currentframe()
assert not cur_frame is None

cmd_folder = os.path.realpath(
    os.path.abspath(
        os.path.split(inspect.getfile(cur_frame))[0]))

if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import crlogger
import crplatform
import crrange
import crcolors

import unohelper
import uno

from com.sun.star.task import XJob
from com.sun.star.awt import XDialogEventHandler, XTextListener, XItemListener, XDialog, XDialogProvider2
from com.sun.star.sheet import XRangeSelectionListener
from com.sun.star.frame.DispatchResultState import SUCCESS
from com.sun.star.sheet.GeneralFunction import MAX as GeneralFunction_MAX
from com.sun.star.sheet.ConditionOperator import FORMULA as ConditionOperator_FORMULA
from com.sun.star.table.CellContentType import TEXT as CellContentType_TEXT

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
        self.dialog: XDialog | None = None
        if not self.testMode:
            self.logger.debug(f"extension path = {self._getExtensionPath()}")

    @staticmethod
    def createInstance(ctx):
        return CRJobImpl(ctx)

    @staticmethod
    def getImplementationName() -> str:
        return "com.github.dennisfrancis.CRJob"

    @staticmethod
    def getServiceNames() -> Tuple[str]:
        return ("com.sun.star.task.Job",)

    def _getExtensionURL(self) -> str:
        piProvider = self.ctx.getByName("/singletons/com.sun.star.deployment.PackageInformationProvider")
        return piProvider.getPackageLocation("com.github.dennisfrancis.ClusterRows")

    def _getExtensionPath(self) -> str:
        extension_uri = self._getExtensionURL()
        return unohelper.fileUrlToSystemPath(extension_uri)

    def _getLogPath(self) -> str:
        return os.path.join("build", self.platvars.osName) if self.testMode else self._getExtensionPath()

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

        dialogProvider: XDialogProvider2 = self.ctx.getServiceManager() \
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

        if self.dialog is None:
            return False
        dlgHandler.setDialog(self.dialog)
        dlgHandler.setupControlHandlers()
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
        self.logger.debug(f"CRJobImpl.execute: envType = {self.envType}, eventName = {self.eventName}, \n\tframe = {self.frame}\n\tmodel = {self.model}")

        if self.eventName == "onClusterRowsReqDialog":
            self._launchClusterDialog()

        return self._getSuccessReturn()

class CellAddress:
    def __init__(self, row: int = -1, col: int = -1, sheet: int = -1):
        self.row = row
        self.col = col
        self.sheet = sheet

class RangeAddress:
    def __init__(self, colStart: int, rowStart: int, colEnd: int, rowEnd: int, sheet: int):
        self.StartColumn = colStart
        self.StartRow = rowStart
        self.EndColumn = colEnd
        self.EndRow = rowEnd
        self.Sheet = sheet

class GMMArgs(object):
    def __init__(self, numClusters: int = 0, numEpochs: int = 10, numIterations: int = 100, colorRows: int = True, hasHeader: bool = False):
        self.rangeAddr: RangeAddress | None = None
        self.outputAddr: CellAddress | None = CellAddress()
        self.numClusters = numClusters
        self.numEpochs = numEpochs
        self.numIterations = numIterations
        self.colorRows = colorRows
        self.hasHeader = hasHeader

    def __str__(self):
        if not self.rangeAddr is None:
            rangeAddrStr = f"\n\t\tcols=[{self.rangeAddr.StartColumn}, {self.rangeAddr.EndColumn}]"
            rangeAddrStr += f"\n\t\trows=[{self.rangeAddr.StartRow}, {self.rangeAddr.EndRow}]\n\t\tsheet={self.rangeAddr.Sheet}"
        else:
            rangeAddrStr = "{UnknownAddress}"
        if self.outputAddr is None:
            outputAddrStr = "{UnknownAddress}"
        else:
            outputAddrStr = f"\n\t\tcol = {self.outputAddr.col}, row = {self.outputAddr.row}, sheet = {self.outputAddr.sheet}"
        paramStr = f"numClusters = {self.numClusters}, numEpochs = {self.numEpochs}, numIterations = {self.numIterations}"
        paramStr += f"\ncolorRows = {self.colorRows}, hasHeader = {self.hasHeader}"
        return f"GMMArgs(\n\trangeAddr({rangeAddrStr}),\n\toutputAddr({outputAddrStr})\n\t{paramStr})"

    def rows(self) -> int:
        """Returns the number of rows in the range"""
        if self.rangeAddr is None:
            return 0
        return self.rangeAddr.EndRow - self.rangeAddr.StartRow + 1

    def drows(self) -> int:
        """Returns the number of data rows"""
        rowCount = self.rows()
        return rowCount - 1 if self.hasHeader else rowCount

    def dcols(self) -> int:
        """Returns the number of data columns"""
        if self.rangeAddr is None:
            return 0
        return self.rangeAddr.EndColumn - self.rangeAddr.StartColumn + 1

    def updateOutputLocation(self):
        if self.rangeAddr is None:
            return

        if self.outputAddr is None:
            self.outputAddr = CellAddress(row = self.rangeAddr.StartRow,
                col = self.rangeAddr.EndColumn + 1, sheet = self.rangeAddr.Sheet)
            return

        self.outputAddr.col = self.rangeAddr.EndColumn + 1
        self.outputAddr.row = self.rangeAddr.StartRow
        self.outputAddr.sheet = self.rangeAddr.Sheet

    def setOutputLocation(self, rangeAddr = None):
        if rangeAddr is None:
            self.outputAddr = None
            return

        if self.outputAddr is None:
            self.outputAddr = CellAddress(row = rangeAddr.StartRow,
                col = rangeAddr.StartRow, sheet = rangeAddr.Sheet)
            return

        self.outputAddr.col = rangeAddr.StartColumn
        self.outputAddr.row = rangeAddr.StartRow
        self.outputAddr.sheet = rangeAddr.Sheet

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
        self.gmmArgs.updateOutputLocation()
        self.rangeListener = None
        self.settingControlValue = False

    def setDialog(self, dialog: XDialog) -> None:
        self.dialog = dialog
        self.setDialogRanges()
        self._markFieldError("LabelText_Error", hasError=True)
        self.validate()

    def _detectHeader(self):
        rangeAddr = self.gmmArgs.rangeAddr
        if not rangeAddr or not self.dialog:
            return
        sheet = self.model.Sheets[rangeAddr.Sheet]
        foundHeader = False
        for col in range(rangeAddr.StartColumn, rangeAddr.EndColumn + 1):
            cell = sheet.getCellByPosition(col, rangeAddr.StartRow)
            if cell.getType() == CellContentType_TEXT:
                foundHeader = True
                break
        self.gmmArgs.hasHeader = foundHeader
        self.dialog.getControl("CheckBox_HasHeader").setState(1 if foundHeader else 0)

    def setDialogRanges(self):
        if not self.dialog:
            return
        self.settingControlValue = True
        # input
        rangeAddr = self.gmmArgs.rangeAddr
        noInput = (rangeAddr is None)
        inputIsSingleCell =  (not noInput) \
            and (rangeAddr.StartColumn == rangeAddr.EndColumn) \
            and (rangeAddr.StartRow == rangeAddr.EndRow)
        if noInput or inputIsSingleCell:
            rangeStr = ""
        else:
            rangeStr = crrange.cellRangeToString(self.gmmArgs.rangeAddr, self.model)
        self.dialog.getControl("TextField_DataRange").setText(rangeStr)
        self._detectHeader()

        # output
        if (self.gmmArgs.outputAddr is None) or inputIsSingleCell or noInput:
            outRangeStr = ""
        else:
            outRangeStr = crrange.cellAddressToString(
                self.gmmArgs.outputAddr.col, self.gmmArgs.outputAddr.row,
                sheet=self.model.Sheets[self.gmmArgs.outputAddr.sheet].Name)
        self.dialog.getControl("TextField_OutputLocation").setText(outRangeStr)
        self.settingControlValue = False

    def getSupportedMethodNames(self):
        return (
            "onOKButtonPress",
            "onCancelButtonPress",
            "onInputFocusLost",
            "onRangeSelButtonPress",
            "onOutputSelButtonPress")

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
                self._onRangeSelButtonPress()
            elif methodName == "onOutputSelButtonPress":
                self._onOutputSelButtonPress()

        except Exception as e:
            self.logger.exception("CRDialogHandler.callHandlerMethod() crashed.")

        return True

    def _onDataRangeSelected(self):
        if self.rangeListener is None or self.dialog is None:
            return
        if not self.rangeListener.failed:
            self.gmmArgs.rangeAddr = crrange.stringToCellRange(self.rangeListener.rangeStr, self.model)
            self.gmmArgs.updateOutputLocation()
            self.setDialogRanges()
            self.validate()

        self.dialog.setVisible(True)

    def _onOutputLocationSelected(self):
        if self.rangeListener is None or self.dialog is None:
            return
        if not self.rangeListener.failed:
            cellRange = crrange.stringToCellRange(self.rangeListener.rangeStr, self.model)
            self.gmmArgs.setOutputLocation(cellRange)
            self.setDialogRanges()
            self.validate()

        self.dialog.setVisible(True)

    def _onRangeSelButtonPress(self):
        if self.dialog is None:
            return
        self.dialog.setVisible(False)
        rangeStr = crrange.cellRangeToString(self.gmmArgs.rangeAddr, self.model)
        self.startRangeSelection(windowTitle = "Select the data cell range", rlId = "input", initialRangeStr=rangeStr)

    def _onOutputSelButtonPress(self):
        if self.dialog is None:
            return
        self.dialog.setVisible(False)
        addr = self.gmmArgs.outputAddr
        if addr is None:
            rangeStr = ""
        else:
            rangeStr = crrange.cellAddressToString(col = addr.col,
                row = addr.row, sheet = self.model.Sheets[addr.sheet].Name)
        self.startRangeSelection(windowTitle = "Select the location to write the results",
            rlId = "output", initialRangeStr=rangeStr)

    def startRangeSelection(self, windowTitle, rlId, initialRangeStr):
        if not self.rangeListener is None:
            self.logger.error("CRDialogHandler.startRangeSelection: rangeListener already exists!")
            return

        self.rangeListener = CRRangeSelectionListener(self, rlId = rlId)

        self.controller.addRangeSelectionListener(self.rangeListener)
        initialValue = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        title = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        closeOnMouseRelease = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        singleCellMode = uno.createUnoStruct("com.sun.star.beans.PropertyValue")
        initialValue.Name = "InitialValue"
        initialValue.Value = initialRangeStr
        title.Name = "Title"
        title.Value = windowTitle
        closeOnMouseRelease.Name = "CloseOnMouseRelease"
        closeOnMouseRelease.Value = True
        singleCellMode.Name = "SingleCellMode"
        singleCellMode.Value = (rlId == "output")

        self.controller.startRangeSelection((initialValue, title, closeOnMouseRelease, singleCellMode))

    def stopRangeSelection(self):
        if self.rangeListener is None:
            self.logger.error("CRDialogHandler.stopRangeSelection: rangeListener is missing!")
            return

        try:
            self.controller.removeRangeSelectionListener(self.rangeListener)

            if self.rangeListener.rlId == "input":
                self._onDataRangeSelected()
            elif self.rangeListener.rlId == "output":
                self._onOutputLocationSelected()
            else:
                self.logger.error(f"CRDialogHandler.stopRangeSelection: invalid rangeListener.rlId = {self.rangeListener.rlId}")

            self.rangeListener = None
        except Exception as e:
            self.logger.exception("CRDialogHandler.stopRangeSelection() crashed")

    def readDialogInputs(self):
        if self.dialog is None:
            return
        # input
        rangeStr = self.dialog.getControl("TextField_DataRange").getText()
        self.gmmArgs.rangeAddr = crrange.stringToCellRange(rangeStr, self.model)

        # output
        rangeStr = self.dialog.getControl("TextField_OutputLocation").getText()
        rangeAddr = crrange.stringToCellRange(rangeStr, self.model)
        self.gmmArgs.setOutputLocation(rangeAddr)

        # other parameters
        self.gmmArgs.hasHeader = bool(self.dialog.getControl("CheckBox_HasHeader").getState())
        self.gmmArgs.numClusters = int(self.dialog.getControl("NumericField_NumClusters").getValue())
        self.gmmArgs.numEpochs = int(self.dialog.getControl("NumericField_NumEpochs").getValue())
        self.gmmArgs.numIterations = int(self.dialog.getControl("NumericField_NumIter").getValue())
        self.gmmArgs.colorRows = bool(self.dialog.getControl("CheckBox_ColorRows").getState())

    def writeResults(self):
        self.readDialogInputs()
        if self.gmmArgs.rangeAddr is None or self.gmmArgs.outputAddr is None or self.dialog is None:
            return
        dataRange = uno.createUnoStruct("com.sun.star.table.CellRangeAddress")
        dataRange.Sheet = self.gmmArgs.rangeAddr.Sheet
        dataRange.StartColumn = self.gmmArgs.rangeAddr.StartColumn
        dataRange.EndColumn = self.gmmArgs.rangeAddr.EndColumn
        dataRange.StartRow = self.gmmArgs.rangeAddr.StartRow + (1 if self.gmmArgs.hasHeader else 0)
        dataRange.EndRow = self.gmmArgs.rangeAddr.EndRow

        resultsRange = uno.createUnoStruct("com.sun.star.table.CellRangeAddress")
        resultsRange.Sheet = self.gmmArgs.outputAddr.sheet
        resultsRange.StartColumn = self.gmmArgs.outputAddr.col
        resultsRange.EndColumn = resultsRange.StartColumn + 1
        resultsRange.StartRow = self.gmmArgs.outputAddr.row + (1 if self.gmmArgs.hasHeader else 0)
        resultsRange.EndRow = resultsRange.StartRow + dataRange.EndRow - dataRange.StartRow

        undoMgr = self.model.getUndoManager()
        if not undoMgr is None:
            undoMgr.enterUndoContext("ClusterRowsImpl_UNDO")

        resRangeObj: Any = crrange.rangeAddressToObject(resultsRange, self.model)
        formulaName = "COM.GITHUB.DENNISFRANCIS.DATACLUSTER.GMMCLUSTER"
        rangeArg = crrange.cellRangeToString(dataRange, self.model)
        args = self.gmmArgs
        resRangeObj.setArrayFormula(
            f"={formulaName}({rangeArg};{args.numClusters};{args.numEpochs};{args.numIterations})")

        if args.hasHeader:
            sheet = self.model.Sheets[resultsRange.Sheet]
            cell = sheet.getCellByPosition(resultsRange.StartColumn, resultsRange.StartRow - 1)
            cell.setFormula("ClusterId")
            cell = sheet.getCellByPosition(resultsRange.StartColumn + 1, resultsRange.StartRow - 1)
            cell.setFormula("Confidence")

        # Find actual number of clusters if in auto mode
        self._updateNumClusters(resRangeObj)

        if self.gmmArgs.colorRows:
            self._addClusterStyles()
            self._colorClusterData(dataRange, resultsRange)

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

    def _colorClusterData(self, dataRange, resultsRange):
        rangeObj: Any = crrange.rangeAddressToObject(dataRange, self.model)
        cfEntries = rangeObj.getPropertyValue("ConditionalFormat")
        if cfEntries is None:
            self.logger.error("CRDialogHandler._colorClusterData: cannot get XSheetConditionalEntries from data range")
            return
        cfEntries.clear()

        sheetName = crrange.sheetName(resultsRange.Sheet, self.model)
        # row is 0 which is relative w.r.t data table's top row
        refCell = f"${sheetName}.$" + crrange.cellAddressToString(
            resultsRange.StartColumn, abs(resultsRange.StartRow - dataRange.StartRow), absolute=False)
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

    def validate(self):
        if self.settingControlValue:
            return
        try:
            self._validate()
        except Exception as e:
            self.logger.exception("CRDialogHandler.validate() crashed.")

    def _validate(self):
        if self.dialog is None:
            return

        self.readDialogInputs()

        if self.dialog.getControl("TextField_DataRange").getText() == "":
            self._setStatus("No input range specified!")
            self._markFieldError("TextField_DataRange")
            return

        if self.gmmArgs.rangeAddr is None:
            self._setStatus("Invalid data range!")
            self._markFieldError("TextField_DataRange")
            return

        # Not enough rows
        if self.gmmArgs.drows() < 10:
            self._setStatus("There must be at least 10 samples")
            self._markFieldError("TextField_DataRange")
            return

        if self.gmmArgs.outputAddr is None:
            self._setStatus("Invalid output location!")
            self._markFieldError("TextField_OutputLocation")
            return

        if self.gmmArgs.colorRows and (self.gmmArgs.rangeAddr.StartRow > self.gmmArgs.outputAddr.row):
            self._setStatus("For coloring rows output location cannot start above(rows) data range")
            self._markFieldError("TextField_OutputLocation")
            return

        self._markFieldError("TextField_DataRange", hasError=False)
        # Range is valid, preserve it by creating a selection.
        selectRange(self.gmmArgs.rangeAddr, self.model, self.logger)

        # No column space to write results
        if self.gmmArgs.outputAddr.col + 1 > MAXCOL:
            self._setStatus("Not enough column space to write the results (2 columns)")
            self._markFieldError("TextField_OutputLocation")
            return

        # No row space to write results
        rows = self.gmmArgs.rows()
        if (self.gmmArgs.outputAddr.row + rows - 1) > MAXROW:
            self._setStatus(f"Not enough row space to write the results ({rows} rows)")
            self._markFieldError("TextField_OutputLocation")
            return

        self._markFieldError("TextField_OutputLocation", hasError=False)

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
        if self.dialog is None:
            return
        hasError = bool(errMsg)
        computeButton = self.dialog.getControl("CommandButton_OK")
        errorLabel = self.dialog.getControl("LabelText_Error")
        computeButton.getModel().setPropertyValue("Enabled", not hasError)
        errorLabel.setText(("Error: " + errMsg) if hasError else errMsg)

    def _markFieldError(self, controlName: str, hasError = True, defColor = None):
        if self.dialog is None:
            return
        model = self.dialog.getControl(controlName).getModel()
        model.setPropertyValue("TextColor", 0xff0000 if hasError else None)

    def setupControlHandlers(self):
        if self.dialog is None:
            return
        self.dataRangeTextListener = CRDataRangeTextListener(self)
        self.dialog.getControl("TextField_DataRange").addTextListener(self.dataRangeTextListener)
        self.dialog.getControl("TextField_OutputLocation").addTextListener(self.dataRangeTextListener)
        self.headerCheckBoxListener = CRHeaderCheckBoxListener(self)
        self.dialog.getControl("CheckBox_HasHeader").addItemListener(self.headerCheckBoxListener)

    def clearDialogControlHandlers(self):
        if self.dialog is None:
            return
        self.dialog.getControl("TextField_DataRange").removeTextListener(self.dataRangeTextListener)
        self.dialog.getControl("TextField_OutputLocation").removeTextListener(self.dataRangeTextListener)
        self.dialog.getControl("CheckBox_HasHeader").removeItemListener(self.headerCheckBoxListener)

# global functions

def selectRange(rangeAddress, document, logger):
    rangeObj = crrange.rangeAddressToObject(rangeAddress, document)
    if rangeObj is None:
        logger.error("global.selectRange: crrange.stringToRangeObj returned None!")
        return
    document.getCurrentController().select(rangeObj)


class CRRangeSelectionListener(unohelper.Base, XRangeSelectionListener):
    def __init__(self, dlgHandler: CRDialogHandler, rlId: str):
        self.dlgHandler = dlgHandler
        self.rlId = rlId
        self.failed = False
        self.rangeStr = ""
        return

    def done(self, event):
        self.rangeStr = event.RangeDescriptor
        self.failed = False
        self.dlgHandler.stopRangeSelection()

    def aborted(self, event):
        self.failed = True
        self.dlgHandler.stopRangeSelection()

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
