#pragma once

namespace util
{
class DataMatrix
{
public:
    /// @brief Accepts a externally owned double array to use it as a column major matrix.
    /// @param pRows externally owned array
    /// @param rows number of rows
    /// @param cols number of columns
    DataMatrix(const double* pRows, const int rows, const int cols)
        : pData(pRows)
        , m_rows(rows)
        , m_cols(cols)
    {
    }

    ~DataMatrix() {}

    const double* operator[](int row) const { return pData + (row * m_cols); }

private:
    const double* pData;
    const int m_rows;
    const int m_cols;
};

}
