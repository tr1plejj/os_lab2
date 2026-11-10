#include "src/experiment.h"

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <random>

#include "src/matrix.h"

namespace lab2 {

std::optional<Matrix> GenerateRandomMatrix(size_t rows, size_t cols,
                                           std::mt19937& generator) {
  std::optional<Matrix> result = Matrix::Create(rows, cols);
  if (!result.has_value()) {
    return std::nullopt;
  }

  std::uniform_int_distribution<int> distribution(0, kMaxRandomValue);
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < cols; ++col) {
      (*result)(row, col) = distribution(generator);
    }
  }
  return result;
}

int64_t CalculateChecksum(const Matrix& matrix) {
  int64_t sum = 0;
  for (size_t row = 0; row < matrix.rows(); ++row) {
    for (size_t col = 0; col < matrix.cols(); ++col) {
      sum += matrix(row, col);
    }
  }
  return sum;
}

}  // namespace lab2
