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

import numpy as np

def genGMM(N = 300, K = 4, p = np.array([[0.1], [0.5], [0.2], [0.2]]), mu = None, sigma = None, fname = "data.csv"):
    #print(p.shape)
    assert(p.shape == (K, 1))
    assert(p.sum() > 0)
    assert(p.min() >= 0)
    assert(not mu is None)
    assert(not sigma is None)

    assert(len(mu) == K)
    assert(len(sigma) == K)
    D = mu[0].shape[0]
    for idx in range(K):
        assert(mu[idx].shape == (D, 1))
        assert(sigma[idx].shape == (D, D))

    p = p / p.sum()
    X = np.empty([N, D+1])
    nc = (p * N).round()
    nc[K-1, 0] = N - nc[0:K-1, 0].sum()
    #print("D = {}, N = {}, K = {}".format(D, N, K))
    #print(nc)

    start_row = 0
    for c in range(K):
        num_rows = int(nc[c])
        end_row = start_row + num_rows # exclusive
        #print("Cluster = {}, start_row = {}, end_row = {}".format(c, start_row, end_row))
        X[start_row:end_row, 0:D] = np.random.multivariate_normal(mu[c].reshape((D,)), sigma[c], size=num_rows)
        X[start_row:end_row, D] = c
        start_row = end_row

    # in-place shuffle of rows
    np.random.shuffle(X)
    header = ",".join(["X{}".format(idx) for idx in range(D)]) + ",ClusterIdx"
    np.savetxt(fname, X, fmt="%.3f", delimiter=",", header=header, comments="")

def gen_demo_data():
    p = np.array([[0.25], [0.25], [0.25], [0.25]])
    mu = [
        np.array([[5], [0]]),
        np.array([[3.0], [2.0]]),
        np.array([[0.0], [0.0]]),
        np.array([[0], [5]])
    ]

    sigma = [
        np.array([[1.0, 0.0], [0.0, 0.25]]),
        np.array([[2.0, 0.6], [0.6, 1.0]]),
        np.array([[1.0, -0.5], [-0.5, 0.8]]),
        np.array([[0.25, 0.0], [0.0, 1.0]])
    ]
    genGMM(p=p, mu=mu, sigma=sigma, fname="data.csv")

if __name__ == '__main__':
    gen_demo_data()