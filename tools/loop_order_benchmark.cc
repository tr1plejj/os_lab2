// Однопоточное сравнение порядков циклов из раздела 2.4 задания.

#include <stddef.h>
#include <stdint.h>

#include <charconv>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <system_error>

#include "src/experiment.h"
#include "src/matrix.h"

namespace {

// Обходит B и C по строкам. Все промежуточные суммы должны входить в int64_t.
std::optional<lab2::Matrix> MultiplyRowMajor(const lab2::Matrix& a,
                                             const lab2::Matrix& b) {
  if (a.cols() != b.rows()) {
    return std::nullopt;
  }
  std::optional<lab2::Matrix> result = lab2::Matrix::Create(a.rows(), b.cols());
  if (!result.has_value()) {
    return std::nullopt;
  }
  lab2::Matrix& product = *result;
  for (size_t i = 0; i < a.rows(); ++i) {
    for (size_t t = 0; t < a.cols(); ++t) {
      const int64_t value = a(i, t);
      for (size_t j = 0; j < b.cols(); ++j) {
        product(i, j) += value * b(t, j);
      }
    }
  }
  return result;
}

// До замера проверяет каждый элемент альтернативного алгоритма на примере.
bool CheckRowMajor() {
  const std::optional<lab2::Matrix> a =
      lab2::Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<lab2::Matrix> b =
      lab2::Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  const std::optional<lab2::Matrix> expected =
      lab2::Matrix::Create(2, 2, {58, 64, 139, 154});
  return a.has_value() && b.has_value() && expected.has_value() &&
         MultiplyRowMajor(*a, *b) == expected;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Использование: " << argv[0] << " N ijt|itj\n";
    return 1;
  }
  size_t n = 0;
  const char* end = argv[1] + std::strlen(argv[1]);
  const std::from_chars_result parsed = std::from_chars(argv[1], end, n);
  const std::string order = argv[2];
  if (parsed.ec != std::errc() || parsed.ptr != end || n == 0 ||
      (order != "ijt" && order != "itj")) {
    std::cerr << "Ошибка: нужен положительный размер и порядок ijt либо itj.\n";
    return 1;
  }
  if (n > std::numeric_limits<int64_t>::max() / 81 / n / n) {
    std::cerr << "Ошибка: контрольная сумма может переполниться.\n";
    return 1;
  }
  if (!CheckRowMajor()) {
    std::cerr << "Ошибка проверки порядка i -> t -> j.\n";
    return 1;
  }

  std::mt19937 generator(42);
  const std::optional<lab2::Matrix> a =
      lab2::GenerateRandomMatrix(n, n, generator);
  const std::optional<lab2::Matrix> b =
      lab2::GenerateRandomMatrix(n, n, generator);
  if (!a.has_value() || !b.has_value()) {
    return 1;
  }
  const std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
  const std::optional<lab2::Matrix> result =
      order == "ijt" ? lab2::MultiplySequential(*a, *b)
                     : MultiplyRowMajor(*a, *b);
  const std::chrono::steady_clock::time_point finish =
      std::chrono::steady_clock::now();
  if (!result.has_value()) {
    return 1;
  }
  std::cout << "Порядок циклов: " << order << '\n'
            << std::fixed << std::setprecision(9) << "Время умножения, с: "
            << std::chrono::duration<double>(finish - start).count() << '\n'
            << "Контрольная сумма: " << lab2::CalculateChecksum(*result)
            << '\n';
  return 0;
}
