#include "src/matrix.h"

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <optional>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

namespace lab2 {
namespace {

// Сравнивает успешные результаты, их размеры и каждый элемент.
void ExpectMatrixEqual(const std::optional<Matrix>& actual,
                       const std::optional<Matrix>& expected) {
  ASSERT_TRUE(actual.has_value());
  ASSERT_TRUE(expected.has_value());
  ASSERT_EQ(actual->rows(), expected->rows());
  ASSERT_EQ(actual->cols(), expected->cols());
  for (size_t row = 0; row < expected->rows(); ++row) {
    for (size_t col = 0; col < expected->cols(); ++col) {
      EXPECT_EQ((*actual)(row, col), (*expected)(row, col))
          << "row=" << row << ", col=" << col;
    }
  }
}

// Проверяет начало с 0%, строгий рост процентов и единственное завершение 100%.
void ExpectCompleteProgress(const std::string& output) {
  std::istringstream lines(output);
  std::string line;
  int previous_percent = -1;
  int report_count = 0;
  while (std::getline(lines, line)) {
    std::istringstream fields(line);
    std::string label;
    int percent = -1;
    char suffix = '\0';
    fields >> label >> percent >> suffix;
    ASSERT_FALSE(fields.fail());
    EXPECT_EQ(label, "Прогресс:");
    EXPECT_EQ(suffix, '%');
    EXPECT_GE(percent, 0);
    EXPECT_LE(percent, 100);
    EXPECT_GT(percent, previous_percent);
    if (report_count == 0) {
      EXPECT_EQ(percent, 0);
    }
    previous_percent = percent;
    ++report_count;
  }
  EXPECT_GE(report_count, 2);
  EXPECT_EQ(previous_percent, 100);
}

TEST(MatrixValidation, RejectsZeroDimensions) {
  EXPECT_FALSE(Matrix::Create(0, 3).has_value());
  EXPECT_FALSE(Matrix::Create(3, 0).has_value());
}

TEST(MatrixValidation, RejectsWrongValueCount) {
  EXPECT_FALSE(Matrix::Create(2, 2, {1, 2, 3}).has_value());
  EXPECT_FALSE(Matrix::Create(1, 1, {1, 2}).has_value());
}

TEST(MatrixValidation, RejectsSizeOverflowBeforeAllocation) {
  EXPECT_FALSE(
      Matrix::Create(std::numeric_limits<size_t>::max(), 2).has_value());
}

TEST(MatrixCreation, EmptyValuesProduceZeros) {
  const std::optional<Matrix> matrix = Matrix::Create(2, 3, {});

  ExpectMatrixEqual(matrix, Matrix::Create(2, 3, {0, 0, 0, 0, 0, 0}));
}

TEST(SequentialMultiplication, OneByOneAndNegativeValue) {
  const std::optional<Matrix> a = Matrix::Create(1, 1, {7});
  const std::optional<Matrix> b = Matrix::Create(1, 1, {-3});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->rows(), 1);
  EXPECT_EQ(result->cols(), 1);
  EXPECT_EQ((*result)(0, 0), -21);
}

TEST(SequentialMultiplication, KnownSquareProduct) {
  const std::optional<Matrix> a = Matrix::Create(2, 2, {1, 2, 3, 4});
  const std::optional<Matrix> b = Matrix::Create(2, 2, {5, 6, 7, 8});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(2, 2, {19, 22, 43, 50}));
}

TEST(SequentialMultiplication, KnownRectangularProduct) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<Matrix> b = Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(2, 2, {58, 64, 139, 154}));
}

TEST(SequentialMultiplication, IdentityOnEitherSide) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {2, 3, 4, 5, 6, 7});
  const std::optional<Matrix> identity2 = Matrix::Create(2, 2, {1, 0, 0, 1});
  const std::optional<Matrix> identity3 =
      Matrix::Create(3, 3, {1, 0, 0, 0, 1, 0, 0, 0, 1});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(identity2.has_value());
  ASSERT_TRUE(identity3.has_value());

  const std::optional<Matrix> left = MultiplySequential(*identity2, *a);
  const std::optional<Matrix> right = MultiplySequential(*a, *identity3);

  ExpectMatrixEqual(left, a);
  ExpectMatrixEqual(right, a);
}

TEST(SequentialMultiplication, ZeroOnEitherSide) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<Matrix> b = Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  const std::optional<Matrix> zero_a = Matrix::Create(2, 3);
  const std::optional<Matrix> zero_b = Matrix::Create(3, 2);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  ASSERT_TRUE(zero_a.has_value());
  ASSERT_TRUE(zero_b.has_value());

  const std::optional<Matrix> left = MultiplySequential(*zero_a, *b);
  const std::optional<Matrix> right = MultiplySequential(*a, *zero_b);

  const std::optional<Matrix> expected = Matrix::Create(2, 2, {0, 0, 0, 0});
  ExpectMatrixEqual(left, expected);
  ExpectMatrixEqual(right, expected);
}

TEST(SequentialMultiplication, RejectsIncompatibleDimensions) {
  const std::optional<Matrix> a = Matrix::Create(2, 3);
  const std::optional<Matrix> b = Matrix::Create(2, 2);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  EXPECT_FALSE(result.has_value());
}

TEST(SequentialMultiplication, SingleRowResult) {
  const std::optional<Matrix> a = Matrix::Create(1, 3, {1, 2, 3});
  const std::optional<Matrix> b = Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(1, 2, {58, 64}));
}

TEST(SequentialMultiplication, SingleColumnResult) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<Matrix> b = Matrix::Create(3, 1, {1, 2, 3});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(2, 1, {14, 32}));
}

TEST(SequentialMultiplication, SingleInnerDimension) {
  const std::optional<Matrix> a = Matrix::Create(3, 1, {2, 3, 4});
  const std::optional<Matrix> b = Matrix::Create(1, 2, {5, 6});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(3, 2, {10, 12, 15, 18, 20, 24}));
}

TEST(SequentialMultiplication, PreservesInputs) {
  const std::optional<Matrix> a = Matrix::Create(2, 2, {1, 2, 3, 4});
  const std::optional<Matrix> b = Matrix::Create(2, 2, {5, 6, 7, 8});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  const std::optional<Matrix> original_a = a;
  const std::optional<Matrix> original_b = b;

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(2, 2, {19, 22, 43, 50}));
  EXPECT_EQ(a, original_a);
  EXPECT_EQ(b, original_b);
}

TEST(SequentialMultiplication, Uses64BitArithmetic) {
  const std::optional<Matrix> a = Matrix::Create(1, 1, {50'000});
  const std::optional<Matrix> b = Matrix::Create(1, 1, {50'000});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplySequential(*a, *b);

  ExpectMatrixEqual(result, Matrix::Create(1, 1, {2'500'000'000}));
}

TEST(ParallelMultiplication, KnownRectangularProduct) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<Matrix> b = Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> expected =
      Matrix::Create(2, 2, {58, 64, 139, 154});
  const size_t thread_counts[] = {1, 2, 3, 7};
  for (size_t thread_count : thread_counts) {
    SCOPED_TRACE(thread_count);
    const std::optional<Matrix> result = MultiplyParallel(*a, *b, thread_count);
    ExpectMatrixEqual(result, expected);
  }
}

TEST(ParallelMultiplication, MatchesSequentialForDifferentThreadCounts) {
  std::optional<Matrix> a = Matrix::Create(10, 4);
  const std::optional<Matrix> b =
      Matrix::Create(4, 3, {-1, 2, 1, 0, 3, 4, 2, -2, 5, 7, 1, 0});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  for (size_t row = 0; row < a->rows(); ++row) {
    for (size_t col = 0; col < a->cols(); ++col) {
      (*a)(row, col) = static_cast<int64_t>(row * a->cols() + col) - 5;
    }
  }

  const std::optional<Matrix> expected = MultiplySequential(*a, *b);
  const size_t thread_counts[] = {1, 2, 3, 7, 13};
  for (size_t thread_count : thread_counts) {
    SCOPED_TRACE(thread_count);
    const std::optional<Matrix> result = MultiplyParallel(*a, *b, thread_count);
    ExpectMatrixEqual(result, expected);
  }
}

TEST(ParallelMultiplication, ZeroAndIdentityMatrices) {
  const std::optional<Matrix> a = Matrix::Create(2, 2, {2, -3, 4, 5});
  const std::optional<Matrix> identity = Matrix::Create(2, 2, {1, 0, 0, 1});
  const std::optional<Matrix> zero = Matrix::Create(2, 2);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(identity.has_value());
  ASSERT_TRUE(zero.has_value());

  const std::optional<Matrix> left_identity =
      MultiplyParallel(*identity, *a, 3);
  const std::optional<Matrix> right_identity =
      MultiplyParallel(*a, *identity, 3);
  const std::optional<Matrix> left_zero = MultiplyParallel(*zero, *a, 3);
  const std::optional<Matrix> right_zero = MultiplyParallel(*a, *zero, 3);

  ExpectMatrixEqual(left_identity, a);
  ExpectMatrixEqual(right_identity, a);
  ExpectMatrixEqual(left_zero, zero);
  ExpectMatrixEqual(right_zero, zero);
}

TEST(ParallelMultiplication, RejectsZeroThreads) {
  const std::optional<Matrix> a = Matrix::Create(2, 2);
  ASSERT_TRUE(a.has_value());

  const std::optional<Matrix> result = MultiplyParallel(*a, *a, 0);

  EXPECT_FALSE(result.has_value());
}

TEST(ParallelMultiplication, RejectsIncompatibleDimensions) {
  const std::optional<Matrix> a = Matrix::Create(2, 3);
  const std::optional<Matrix> b = Matrix::Create(2, 2);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplyParallel(*a, *b, 3);

  EXPECT_FALSE(result.has_value());
}

TEST(ParallelMultiplication, OneByOneAndNegativeValue) {
  const std::optional<Matrix> a = Matrix::Create(1, 1, {7});
  const std::optional<Matrix> b = Matrix::Create(1, 1, {-3});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplyParallel(*a, *b, 7);

  ExpectMatrixEqual(result, Matrix::Create(1, 1, {-21}));
}

TEST(ParallelMultiplication, SingleRowAndSingleColumn) {
  const std::optional<Matrix> row = Matrix::Create(1, 3, {1, 2, 3});
  const std::optional<Matrix> column = Matrix::Create(3, 1, {4, 5, 6});
  ASSERT_TRUE(row.has_value());
  ASSERT_TRUE(column.has_value());

  const std::optional<Matrix> scalar = MultiplyParallel(*row, *column, 7);
  const std::optional<Matrix> outer = MultiplyParallel(*column, *row, 7);

  ExpectMatrixEqual(scalar, Matrix::Create(1, 1, {32}));
  ExpectMatrixEqual(outer,
                    Matrix::Create(3, 3, {4, 8, 12, 5, 10, 15, 6, 12, 18}));
}

TEST(ParallelMultiplication, PreservesInputs) {
  const std::optional<Matrix> a = Matrix::Create(2, 3, {1, 2, 3, 4, 5, 6});
  const std::optional<Matrix> b = Matrix::Create(3, 2, {7, 8, 9, 10, 11, 12});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  const std::optional<Matrix> original_a = a;
  const std::optional<Matrix> original_b = b;

  const std::optional<Matrix> result = MultiplyParallel(*a, *b, 3);

  ExpectMatrixEqual(result, Matrix::Create(2, 2, {58, 64, 139, 154}));
  EXPECT_EQ(a, original_a);
  EXPECT_EQ(b, original_b);
}

TEST(ParallelMultiplication, Uses64BitArithmetic) {
  const std::optional<Matrix> a = Matrix::Create(1, 1, {50'000});
  const std::optional<Matrix> b = Matrix::Create(1, 1, {50'000});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  const std::optional<Matrix> result = MultiplyParallel(*a, *b, 2);

  ExpectMatrixEqual(result, Matrix::Create(1, 1, {2'500'000'000}));
}

TEST(ProgressReporting, CompletesForDifferentThreadCounts) {
  std::optional<Matrix> a = Matrix::Create(10, 3);
  const std::optional<Matrix> b = Matrix::Create(3, 2, {1, 2, 3, 4, 5, 6});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  for (size_t row = 0; row < a->rows(); ++row) {
    for (size_t col = 0; col < a->cols(); ++col) {
      (*a)(row, col) = static_cast<int64_t>(row + col + 1);
    }
  }
  const std::optional<Matrix> expected = MultiplySequential(*a, *b);

  const size_t thread_counts[] = {1, 2, 3, 7, 13};
  for (size_t thread_count : thread_counts) {
    SCOPED_TRACE(thread_count);
    std::ostringstream progress;
    const std::optional<Matrix> result =
        MultiplyParallel(*a, *b, thread_count, &progress);

    ExpectMatrixEqual(result, expected);
    ExpectCompleteProgress(progress.str());
  }
}

TEST(ProgressReporting, InvalidArgumentsProduceNoProgress) {
  const std::optional<Matrix> a = Matrix::Create(2, 3);
  const std::optional<Matrix> b = Matrix::Create(3, 2);
  const std::optional<Matrix> incompatible = Matrix::Create(2, 2);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  ASSERT_TRUE(incompatible.has_value());
  std::ostringstream progress;

  const std::optional<Matrix> zero_threads =
      MultiplyParallel(*a, *b, 0, &progress);
  const std::optional<Matrix> wrong_dimensions =
      MultiplyParallel(*a, *incompatible, 2, &progress);

  EXPECT_FALSE(zero_threads.has_value());
  EXPECT_FALSE(wrong_dimensions.has_value());
  EXPECT_TRUE(progress.str().empty());
}

}  // namespace
}  // namespace lab2
