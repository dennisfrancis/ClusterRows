#include "matrix.hxx"
#include "diagonal.hxx"

namespace util
{

class SVD
{
private:
    Matrix U;
    DiagonalMatrix S;
    Matrix V;
};
