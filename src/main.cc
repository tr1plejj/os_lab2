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
#include <system_error>

#include "src/experiment.h"
#include "src/matrix.h"

namespace {

// Принимает только положительное целое число без лишних символов.
bool ParsePositiveSize(const char* text, size_t& value) {
  const char* end = text + std::strlen(text);
  const std::from_chars_result parsed = std::from_chars(text, end, value);
  return parsed.ec == std::errc() && parsed.ptr == end && value > 0;
}

// Выводит небольшую матрицу для ручной проверки результата.
void PrintMatrix(const char* name, const lab2::Matrix& matrix) {
  std::cout << name << " (" << matrix.rows() << 'x' << matrix.cols() << "):\n";
  for (size_t row = 0; row < matrix.rows(); ++row) {
    for (size_t col = 0; col < matrix.cols(); ++col) {
      if (col != 0) {
        std::cout << ' ';
      }
      std::cout << matrix(row, col);
    }
    std::cout << '\n';
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "Использование: " << argv[0] << " n m k T\n"
              << "A: n x m, B: m x k; все параметры — положительные целые.\n";
    return 1;
  }

  size_t n = 0;
  size_t m = 0;
  size_t k = 0;
  size_t thread_count = 0;
  if (!ParsePositiveSize(argv[1], n) || !ParsePositiveSize(argv[2], m) ||
      !ParsePositiveSize(argv[3], k) ||
      !ParsePositiveSize(argv[4], thread_count)) {
    std::cerr << "Ошибка: n, m, k и T должны быть положительными целыми.\n";
    return 1;
  }

  // Для значений 0..9 сумма всех элементов C не превосходит 81 * n * m * k.
  // Деление проверяет этот предел без переполнения самого произведения.
  const size_t max_product = lab2::kMaxRandomValue * lab2::kMaxRandomValue;
  if (n > std::numeric_limits<int64_t>::max() / max_product / m / k) {
    std::cerr
        << "Ошибка: размеры слишком велики для контрольной суммы int64_t.\n";
    return 1;
  }

  const unsigned int seed = 42;
  std::mt19937 generator(seed);
  const std::optional<lab2::Matrix> a =
      lab2::GenerateRandomMatrix(n, m, generator);
  const std::optional<lab2::Matrix> b =
      lab2::GenerateRandomMatrix(m, k, generator);
  if (!a.has_value() || !b.has_value()) {
    std::cerr << "Ошибка: недопустимые размеры исходных матриц.\n";
    return 1;
  }

  std::cout << "A: " << n << 'x' << m << ", B: " << m << 'x' << k
            << ", рабочих потоков: " << thread_count << '\n'
            << "Значения: 0.." << lab2::kMaxRandomValue << ", seed: " << seed
            << '\n';
  const std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
  const std::optional<lab2::Matrix> result =
      lab2::MultiplyParallel(*a, *b, thread_count, &std::cout);
  const std::chrono::steady_clock::time_point finish =
      std::chrono::steady_clock::now();
  if (!result.has_value()) {
    std::cerr << "Ошибка: недопустимые размеры результата или число потоков.\n";
    return 1;
  }

  const double seconds = std::chrono::duration<double>(finish - start).count();
  std::cout << std::fixed << std::setprecision(9)
            << "Время умножения, с: " << seconds << '\n'
            << "Контрольная сумма: " << lab2::CalculateChecksum(*result)
            << '\n';
  if (a->rows() <= 10 && a->cols() <= 10) {
    PrintMatrix("A", *a);
  }
  if (b->rows() <= 10 && b->cols() <= 10) {
    PrintMatrix("B", *b);
  }
  if (result->rows() <= 10 && result->cols() <= 10) {
    PrintMatrix("C = A * B", *result);
  }
  return 0;
}
