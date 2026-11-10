// Плотные целочисленные матрицы, последовательное и многопоточное умножение.

#ifndef LAB2_SRC_MATRIX_H_
#define LAB2_SRC_MATRIX_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <ostream>
#include <vector>

namespace lab2 {

// Хранит элементы по строкам. Разные элементы можно обрабатывать параллельно,
// если в это время матрица не присваивается, не перемещается и не уничтожается.
// После перемещения объект допускает только присваивание или уничтожение.
class Matrix {
 public:
  // Создает матрицу из значений по строкам; пустой вектор означает нули.
  // Возвращает nullopt при нулевых или чрезмерных размерах либо неверном
  // количестве значений.
  static std::optional<Matrix> Create(size_t rows, size_t cols,
                                      std::vector<int64_t> values = {});

  Matrix(const Matrix&) = default;
  Matrix& operator=(const Matrix&) = default;
  Matrix(Matrix&&) = default;
  Matrix& operator=(Matrix&&) = default;

  size_t rows() const { return rows_; }
  size_t cols() const { return cols_; }

  // Предоставляет доступ без проверки границ. Требует row < rows() и
  // col < cols(), как vector::operator[] требует допустимого индекса.
  int64_t& operator()(size_t row, size_t col) {
    return values_[row * cols_ + col];
  }
  const int64_t& operator()(size_t row, size_t col) const {
    return values_[row * cols_ + col];
  }

  // Сравнивает размеры и все элементы матриц.
  friend bool operator==(const Matrix& lhs, const Matrix& rhs) = default;

 private:
  // Принимает уже проверенные размеры и соответствующее им хранилище.
  Matrix(size_t rows, size_t cols, std::vector<int64_t> values);

  size_t rows_;
  size_t cols_;
  std::vector<int64_t> values_;
};

// Возвращает произведение или nullopt при несовместимых размерах либо
// чрезмерном размере результата. Произведения и промежуточные суммы должны
// входить в int64_t.
std::optional<Matrix> MultiplySequential(const Matrix& a, const Matrix& b);

// Умножает матрицы в thread_count потоках с непрерывными блоками строк.
// Дожидается всех потоков и возвращает полную матрицу результата. Возвращает
// nullopt при нуле потоков или недопустимых размерах. Произведения и
// промежуточные суммы должны входить в int64_t. Если progress_output не
// nullptr, выводит прогресс в этот поток; объект вывода должен жить до конца
// вызова.
std::optional<Matrix> MultiplyParallel(const Matrix& a, const Matrix& b,
                                       size_t thread_count,
                                       std::ostream* progress_output = nullptr);

}  // namespace lab2

#endif  // LAB2_SRC_MATRIX_H_
