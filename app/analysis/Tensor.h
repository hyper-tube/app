#pragma once

#include <QList>

namespace analysis {

class Tensor
{
public:
    Tensor() = default;
    Tensor(int planes, int rows, int columns);

    void resize(int planes, int rows, int columns);
    void reshape(int planes, int rows, int columns);

    int planes() const { return m_planes; }
    int rows() const { return m_rows; }
    int columns() const { return m_columns; }
    int planeSize() const { return m_rows * m_columns; }
    int size() const { return m_planes * m_rows * m_columns; }

    float *data() { return m_values.data(); }
    const float *data() const { return m_values.constData(); }
    float *plane(int index) { return m_values.data() + qsizetype(index) * planeSize(); }
    const float *plane(int index) const
    {
        return m_values.constData() + qsizetype(index) * planeSize();
    }

private:
    int m_planes = 0;
    int m_rows = 0;
    int m_columns = 0;
    QList<float> m_values;
};

}
