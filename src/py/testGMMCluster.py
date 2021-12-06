from __future__ import print_function
import sys
import os
import inspect
import random

sys.path.append('/usr/lib/libreoffice/program')

cmd_folder = os.path.realpath(
    os.path.abspath(
        os.path.split(inspect.getfile(inspect.currentframe()))[0]))

if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import GMMCluster

def main():
    gmmImpl = GMMCluster.GMMClusterImpl({}, testMode=True)
    data = tuple(((random.uniform(0,10), random.uniform(20, 30)) for i in range(100)))
    ret = gmmImpl.gmmCluster(data, numClusters=3, numEpochs=20, numIterations=100)
    print(ret)

if __name__ == '__main__':
    main()
