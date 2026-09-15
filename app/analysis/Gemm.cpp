#include "Gemm.h"

#include <QtGlobal>

#include <algorithm>

namespace {

constexpr int kTileRows = 4;
constexpr int kTileColumns = 8;

void accumulateTile(const float *left, const float *right, float *result, int height, int inner,
                    int columns)
{
    float tile[kTileRows][kTileColumns] = {};
    for (int step = 0; step < inner; ++step) {
        const float *values = right + qsizetype(step) * columns;
        for (int row = 0; row < kTileRows; ++row) {
            const float scale = row < height ? left[qsizetype(row) * inner + step] : 0.0f;
            for (int column = 0; column < kTileColumns; ++column)
                tile[row][column] += scale * values[column];
        }
    }
    for (int row = 0; row < height; ++row) {
        float *target = result + qsizetype(row) * columns;
        for (int column = 0; column < kTileColumns; ++column)
            target[column] += tile[row][column];
    }
}

void accumulateRest(const float *left, const float *right, float *result, int height, int inner,
                    int columns, int first)
{
    for (int step = 0; step < inner; ++step) {
        const float *values = right + qsizetype(step) * columns;
        for (int row = 0; row < height; ++row) {
            const float scale = left[qsizetype(row) * inner + step];
            float *target = result + qsizetype(row) * columns;
            for (int column = first; column < columns; ++column)
                target[column] += scale * values[column];
        }
    }
}

void multiplyInto(const float *left, const float *right, float *result, int rows, int inner,
                  int columns)
{
    for (int block = 0; block < rows; block += kTileRows) {
        const int height = qMin(kTileRows, rows - block);
        const float *rowsLeft = left + qsizetype(block) * inner;
        float *rowsResult = result + qsizetype(block) * columns;
        int column = 0;
        for (; column + kTileColumns <= columns; column += kTileColumns)
            accumulateTile(rowsLeft, right + column, rowsResult + column, height, inner, columns);
        if (column < columns)
            accumulateRest(rowsLeft, right, rowsResult, height, inner, columns, column);
    }
}

}

namespace analysis {

void Gemm::multiply(const float *left, const float *right, float *result, int rows, int inner,
                    int columns)
{
    std::fill_n(result, qsizetype(rows) * columns, 0.0f);
    multiplyInto(left, right, result, rows, inner, columns);
}

void Gemm::multiplyBiased(const float *left, const float *right, const float *bias, float *result,
                          int rows, int inner, int columns)
{
    for (int row = 0; row < rows; ++row)
        std::copy_n(bias, columns, result + qsizetype(row) * columns);
    multiplyInto(left, right, result, rows, inner, columns);
}

}
