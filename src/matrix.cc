#include "src/matrix.h"

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <chrono>
#include <optional>
#include <ostream>
#include <thread>
#include <utility>
#include <vector>

namespace lab2 {
namespace {

// Выводит процент и сразу передает текст получателю, не ожидая заполнения
// буфера.
void PrintProgress(std::ostream& output, size_t percent) {
  output << "Прогресс: " << percent << "%\n";
  output.flush();
}

// Выводит изменившийся промежуточный процент не чаще раза в 250 мс.
// Требует total_rows > 0 и увеличения счетчика до total_rows рабочими потоками.
void ReportProgress(const std::atomic<size_t>& completed_rows,
                    size_t total_rows, std::ostream& output) {
  auto next_update =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  size_t previous_percent = 0;
  size_t completed = completed_rows.load();
  while (completed < total_rows) {
    const auto now = std::chrono::steady_clock::now();
    const size_t percent = completed * 100 / total_rows;
    if (now >= next_update && percent != previous_percent) {
      PrintProgress(output, percent);
      previous_percent = percent;
      next_update = now + std::chrono::milliseconds(250);
    }
    completed_rows.wait(completed);
    completed = completed_rows.load();
  }
}

// Записывает одну строку произведения. Требует совместимых размеров матриц
// и допустимого номера строки. Каждую строку должен считать один поток.
void MultiplyRow(const Matrix& a, const Matrix& b, size_t row, Matrix& result) {
  for (size_t col = 0; col < b.cols(); ++col) {
    int64_t sum = 0;
    for (size_t t = 0; t < a.cols(); ++t) {
      sum += a(row, t) * b(t, col);
    }
    result(row, col) = sum;
  }
}

}  // namespace

std::optional<Matrix> Matrix::Create(size_t rows, size_t cols,
                                     std::vector<int64_t> values) {
  if (rows == 0 || cols == 0) {
    return std::nullopt;
  }
  // Деление позволяет проверить размер до возможного переполнения rows * cols.
  if (rows > values.max_size() / cols) {
    return std::nullopt;
  }

  const size_t count = rows * cols;
  if (values.empty()) {
    values.resize(count, 0);
  } else if (values.size() != count) {
    return std::nullopt;
  }
  return Matrix(rows, cols, std::move(values));
}

Matrix::Matrix(size_t rows, size_t cols, std::vector<int64_t> values)
    : rows_(rows), cols_(cols), values_(std::move(values)) {}

std::optional<Matrix> MultiplySequential(const Matrix& a, const Matrix& b) {
  if (a.cols() != b.rows()) {
    return std::nullopt;
  }

  std::optional<Matrix> result = Matrix::Create(a.rows(), b.cols());
  if (!result.has_value()) {
    return std::nullopt;
  }

  Matrix& product = *result;
  for (size_t i = 0; i < a.rows(); ++i) {
    for (size_t j = 0; j < b.cols(); ++j) {
      int64_t sum = 0;
      for (size_t t = 0; t < a.cols(); ++t) {
        sum += a(i, t) * b(t, j);
      }
      product(i, j) = sum;
    }
  }
  return result;
}

std::optional<Matrix> MultiplyParallel(const Matrix& a, const Matrix& b,
                                       size_t thread_count,
                                       std::ostream* progress_output) {
  if (thread_count == 0 || a.cols() != b.rows()) {
    return std::nullopt;
  }

  std::optional<Matrix> result = Matrix::Create(a.rows(), b.cols());
  if (!result.has_value()) {
    return std::nullopt;
  }

  Matrix& product = *result;
  std::atomic<size_t> completed_rows{0};
  if (progress_output != nullptr) {
    PrintProgress(*progress_output, 0);
  }
  const size_t rows_per_thread = a.rows() / thread_count;
  const size_t extra_rows = a.rows() % thread_count;
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  size_t begin = 0;
  for (size_t index = 0; index < thread_count; ++index) {
    const size_t row_count = rows_per_thread + (index < extra_rows ? 1 : 0);
    const size_t end = begin + row_count;
    // Границы копируются: каждый поток сохраняет свой диапазон [begin, end).
    workers.emplace_back([&a, &b, &product, &completed_rows, begin, end] {
      for (size_t row = begin; row < end; ++row) {
        MultiplyRow(a, b, row, product);
        completed_rows.fetch_add(1);
        completed_rows.notify_one();
      }
    });
    begin = end;
  }
  if (progress_output != nullptr) {
    ReportProgress(completed_rows, a.rows(), *progress_output);
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
  if (progress_output != nullptr) {
    PrintProgress(*progress_output, 100);
  }
  return result;
}

}  // namespace lab2
