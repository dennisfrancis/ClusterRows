#include <memory>

namespace util
{

class DiagonalMatrix
{
public:
    explicit DiagonalMatrix(const int size)
        : m_data(std::make_unique<double[]>(size))
        , m_size(size)
    {
    }

    [[nodiscard]] const double& at(int index) const
    {
        check_bounds(index);
        return m_data[index];
    }
    double& at(int index)
    {
        check_bounds(index);
        return m_data[index];
    }

private:
    std::unique_ptr<double[]> m_data;
    const int m_size;

    void check_bounds(int index) const;
};

}