
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

import unohelper

from com.github.dennisfrancis import XGMMCluster

class GMMClusterImpl(unohelper.Base, XGMMCluster):
    """Implementation of GMMCluster addin"""
    def __init__(self):
        return

    @staticmethod
    def _isNumeric(param: object) -> bool:
        return (type(param) == int) or (type(param) == float)

    def gmmCluster(self, data, numClusters, numEpochs, numIterations):
        """Compute clusters for each row of input data matrix with
        the given parameters"""
        if numClusters is None: numClusters = 0
        if numEpochs is None: numEpochs = 10
        if numIterations is None: numIterations = 100
        print('data = {}\nnumClusters = {}\nnumEpochs = {}\nnumIterations = {}'
            .format(data, numClusters, numEpochs, numIterations))
        if (not GMMClusterImpl._isNumeric(numClusters)) \
            or (not GMMClusterImpl._isNumeric(numEpochs)) \
                or (not GMMClusterImpl._isNumeric(numIterations)):
                    return ((-1, 0),)
        return ((1.0, 0.75), (2.0, 0.25))

def createInstance(ctx):
    return GMMClusterImpl()

g_ImplementationHelper = unohelper.ImplementationHelper()
g_ImplementationHelper.addImplementation( \
    createInstance,'com.github.dennisfrancis.python.GMMClusterImpl',
        ('com.sun.star.sheet.AddIn',),)
