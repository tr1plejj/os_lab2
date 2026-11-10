// Генерация исходных данных и контрольная сумма для вычислительных запусков.

#ifndef LAB2_SRC_EXPERIMENT_H_
#define LAB2_SRC_EXPERIMENT_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <random>

#include "src/matrix.h"

namespace lab2 {

inline constexpr int kMaxRandomValue = 9;

// Создает матрицу со случайными целыми значениями от 0 до kMaxRandomValue.
// Возвращает nullopt при недопустимых размерах, как Matrix::Create.
std::optional<Matrix> GenerateRandomMatrix(size_t rows, size_t cols,
                                           std::mt19937& generator);

// Суммирует все элементы. Промежуточные суммы должны помещаться в int64_t.
int64_t CalculateChecksum(const Matrix& matrix);

}  // namespace lab2

#endif  // LAB2_SRC_EXPERIMENT_H_
