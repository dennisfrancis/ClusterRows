#include "diagonal.hxx"

namespace util
{

void DiagonalMatrix::check_bounds(int size) const
{
    if (size < 0 || size >= m_size)
        throw std::out_of_range("Out of bounds element access");
}

}