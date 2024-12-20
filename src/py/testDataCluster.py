from __future__ import print_function
import sys
import os
import inspect
import random

from typing import Tuple

sys.path.append("/usr/lib/libreoffice/program")

curFrame = inspect.currentframe()
assert not curFrame is None

cmd_folder: str = os.path.realpath(
    os.path.abspath(
        os.path.split(inspect.getfile(curFrame))[0]))

if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import DataCluster

def main():
    clusterImpl = DataCluster.DataClusterImpl({}, testMode=True)
    data: Tuple[Tuple[float, float]] = tuple(((random.uniform(0,10), random.uniform(20, 30)) for i in range(100)))
    ret = clusterImpl.gmmCluster(data, numClusters=3, numEpochs=20, numIterations=100, fullGMM=0)
    print(ret)

if __name__ == "__main__":
    main()
