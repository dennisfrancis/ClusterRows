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

from typing import List
import math

def getClusterColors(numClusters: int) -> List[int]:
    hueSlice = 360.0 / numClusters
    L = 0.5
    S = 0.75
    colors = [0] * numClusters
    for idx in range(numClusters):
        # https://en.wikipedia.org/wiki/HSL_and_HSV
        # HSL to RGB conversion formula.

        # Cluster hue
        H = hueSlice * idx
        # Cluster chroma
        C = (1 - abs(2 * L - 1)) * S
        HP = H / 60.0
        X = C * (1 - abs((HP % 2) - 1))
        HPI = math.floor(HP)
        if HPI == 0:
            R, G, B = C, X, 0
        elif HPI == 1:
            R, G, B = X, C, 0
        elif HPI == 2:
            R, G, B = 0, C, X
        elif HPI == 3:
            R, G, B = 0, X, C
        elif HPI == 4:
            R, G, B = X, 0, C
        elif HPI == 5:
            R, G, B = C, 0, X
        else:
            R, G, B = 0, 0, 0

        M = L - C / 2
        R = int((R + M) * 255)
        G = int((G + M) * 255)
        B = int((B + M) * 255)
        colors[idx] = (R << 16 | B << 8 | G)

    return colors
