#include "matrix.hxx"

#include "stdexcept"

namespace util
{

Matrix Matrix::dot(const Matrix& right) const
{
    if (m_cols != right.m_rows)
    {
        throw std::runtime_error("dot: A.cols != B.rows");
    }
    Matrix res(m_rows, right.m_cols);

    for (int row = 0; row < m_rows; ++row)
    {
        for (int col = 0; col < right.m_cols; ++col)
        {
            for (int inner = 0; inner < m_cols; ++inner)
                res.at(row, col) = at(row, inner) * at(inner, col);
        }
    }
    return res;
}

}
