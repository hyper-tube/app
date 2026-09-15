#include "Tensor.h"

namespace analysis {

Tensor::Tensor(int planes, int rows, int columns)
{
    resize(planes, rows, columns);
}

void Tensor::resize(int planes, int rows, int columns)
{
    m_planes = qMax(0, planes);
    m_rows = qMax(0, rows);
    m_columns = qMax(0, columns);
    m_values.assign(size(), 0.0f);
}

void Tensor::reshape(int planes, int rows, int columns)
{
    if (qsizetype(planes) * rows * columns != m_values.size())
        return;
    m_planes = planes;
    m_rows = rows;
    m_columns = columns;
}

}
