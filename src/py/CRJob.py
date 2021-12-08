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

import unohelper
import uno
from com.sun.star.task import XJob
from com.sun.star.awt import XDialogEventHandler
from com.sun.star.frame.DispatchResultState import SUCCESS
from com.sun.star.sheet.CellFlags import VALUE, DATETIME

class CRJobImpl(unohelper.Base, XJob, XDialogEventHandler):
    def __init__(self, ctx, testMode=False):
        self.ctx = ctx
        self.testMode = testMode
        self.platvars = crplatform.CRPlatForm()
        self.logger = crlogger.setupLogger(self._getLogPath())
        self.logger.debug("INIT CRJobImpl")
        self.logger.debug(self.platvars)
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
        self.dataRange = userSelection.getRangeAddress()
        return True

    def _getControl(self, name):
        if not self.dialog:
            return None
        return self.dialog.getControl(name)

    # XDialogEventHandler
    def getSupportedMethodNames(self):
        return (
            "onOKButtonPress",
            "onCancelButtonPress",
            "onInputChange",
            "onInputFocusLost")

    def callHandlerMethod(self, dialog, eventObject, methodName):
        if methodName == "onOKButtonPress":
            print("onOKButtonPress")
        elif methodName == "onCancelButtonPress":
            print("onCancelButtonPress")
            self.dialog.endExecute()
        elif methodName == "onInputFocusLost":
            print("onInputFocusLost")
        return True

    def _createDialogAndExecute(self):
        dialogProvider = self.ctx.getServiceManager() \
            .createInstanceWithContext("com.sun.star.awt.DialogProvider2", self.ctx)
        if dialogProvider is None:
            self.logger.error("CRJobImpl._createDialogAndExecute: cannot create dialogProvider!")
            return False

        xdlFile = self._getExtensionURL() + "/ClusterRows.xdl"
        self.dialog = dialogProvider.createDialogWithHandler(xdlFile, self)
        if self.dialog is None:
            self.logger.error("CRJobImpl._createDialogAndExecute: cannot create dialog!")
            return False

        label = self._getControl("LabelField_DataRange")
        if label is None:
            self.logger.error("CRJobImpl._createDialogAndExecute: cannot get DataRange label from dialog")
            return False

        self.dialog.execute()

        return True

    def _launchClusterDialog(self):
        if not self._getDataRange():
            self.logger.error("CRJobImpl._launchClusterDialog: getDataRange failed!")
            return
        self._createDialogAndExecute()

    def execute(self, args):
        ret = ()
        try:
            ret = self._execute(args)
        except Exception as e:
            self.logger.exception("CRJobImpl._execute() crashed.")
        return ret

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
