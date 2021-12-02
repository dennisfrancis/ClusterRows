
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

import ctypes
from itertools import chain
from typing import Tuple
import time
import unohelper

from com.github.dennisfrancis import XGMMCluster

class GMMClusterImpl(unohelper.Base, XGMMCluster):
    """Implementation of GMMCluster addin"""
    def __init__(self):
        return

    @staticmethod
    def _isNumeric(param: object) -> bool:
        return (type(param) == int) or (type(param) == float)

    def gmmCluster(self, data: Tuple[Tuple[float]], numClusters, numEpochs, numIterations):
        """Compute clusters for each row of input data matrix with
        the given parameters"""
        start = time.time()
        if numClusters is None: numClusters = 0
        if numEpochs is None: numEpochs = 10
        if numIterations is None: numIterations = 100
        #print('data = {}\nnumClusters = {}\nnumEpochs = {}\nnumIterations = {}'
        #    .format(data, numClusters, numEpochs, numIterations))
        if (not GMMClusterImpl._isNumeric(numClusters)) \
            or (not GMMClusterImpl._isNumeric(numEpochs)) \
                or (not GMMClusterImpl._isNumeric(numIterations)) \
                    or (not isinstance(data, tuple)) or len(data) == 0 \
                        or (not isinstance(data[0], tuple)):
                            return ((-1, 0),)
        nrows = len(data)
        ncols = len(data[0])
        startArr = time.time()
        arr = (ctypes.c_double * (ncols * nrows))(*chain.from_iterable(data))
        print(f'arr gen time: {time.time() - startArr}')
        labels = (ctypes.c_int * nrows)()
        confidences = (ctypes.c_double * nrows)()
        gmmModule = ctypes.CDLL('libgmm.so')
        gmm = gmmModule.gmm
        gmm.argtypes = [
            ctypes.POINTER(ctypes.c_double), # data
            ctypes.c_int, # rows
            ctypes.c_int, # cols
            ctypes.c_int, # numClusters
            ctypes.c_int, # numEpochs
            ctypes.c_int, # numIterations
            ctypes.POINTER(ctypes.c_int), # clusterLabels
            ctypes.POINTER(ctypes.c_double), # labelConfidence
        ]
        gmm.restype = ctypes.c_int
        startGmm = time.time()
        _ = gmm(arr, nrows, ncols, int(numClusters), int(numEpochs), int(numIterations), labels, confidences)
        print(f'gmm time: {time.time() - startGmm}')
        #print('gmm status = {}'.format(status))
        startRes = time.time()
        res = tuple(zip(labels, confidences))
        print(f'res gen time: {time.time() - startRes}')
        print(f'Total time: {time.time() - start}')
        return res

def createInstance(ctx):
    return GMMClusterImpl()

g_ImplementationHelper = unohelper.ImplementationHelper()
g_ImplementationHelper.addImplementation( \
    createInstance,'com.github.dennisfrancis.python.GMMClusterImpl',
        ('com.sun.star.sheet.AddIn',),)
