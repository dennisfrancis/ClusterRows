#pragma once
#include "matrix.hxx"
#include "diagonal.hxx"

namespace util
{

struct SVD
{
    explicit SVD(const Matrix& A);

    Matrix U;
    DiagonalMatrix S;
    Matrix V;
};

}