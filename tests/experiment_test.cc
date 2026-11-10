#include "src/experiment.h"

#include <stddef.h>

#include <optional>
#include <random>

#include "gtest/gtest.h"
#include "src/matrix.h"

namespace lab2 {
namespace {

TEST(RandomGeneration, PreservesShapeAndValueRange) {
  std::mt19937 generator(42);

  const std::optional<Matrix> matrix = GenerateRandomMatrix(7, 13, generator);

  ASSERT_TRUE(matrix.has_value());
  ASSERT_EQ(matrix->rows(), 7);
  ASSERT_EQ(matrix->cols(), 13);
  for (size_t row = 0; row < matrix->rows(); ++row) {
    for (size_t col = 0; col < matrix->cols(); ++col) {
      EXPECT_GE((*matrix)(row, col), 0);
      EXPECT_LE((*matrix)(row, col), kMaxRandomValue);
    }
  }
}

TEST(RandomGeneration, SameSeedReproducesBothInputMatrices) {
  std::mt19937 first_generator(42);
  std::mt19937 second_generator(42);

  const std::optional<Matrix> first_a =
      GenerateRandomMatrix(2, 3, first_generator);
  const std::optional<Matrix> first_b =
      GenerateRandomMatrix(3, 2, first_generator);
  const std::optional<Matrix> second_a =
      GenerateRandomMatrix(2, 3, second_generator);
  const std::optional<Matrix> second_b =
      GenerateRandomMatrix(3, 2, second_generator);

  ASSERT_TRUE(first_a.has_value());
  ASSERT_TRUE(first_b.has_value());
  EXPECT_EQ(first_a, second_a);
  EXPECT_EQ(first_b, second_b);
}

TEST(RandomGeneration, RejectsInvalidDimensions) {
  std::mt19937 generator(42);

  EXPECT_FALSE(GenerateRandomMatrix(0, 3, generator).has_value());
  EXPECT_FALSE(GenerateRandomMatrix(3, 0, generator).has_value());
}

TEST(Checksum, SumsEveryElementIncludingNegativeValues) {
  const std::optional<Matrix> matrix =
      Matrix::Create(2, 3, {1, -2, 3, 4, -5, 6});
  ASSERT_TRUE(matrix.has_value());

  EXPECT_EQ(CalculateChecksum(*matrix), 7);
}

TEST(Checksum, Uses64BitAccumulator) {
  const std::optional<Matrix> matrix =
      Matrix::Create(1, 2, {2'000'000'000, 2'000'000'000});
  ASSERT_TRUE(matrix.has_value());

  EXPECT_EQ(CalculateChecksum(*matrix), 4'000'000'000);
}

}  // namespace
}  // namespace lab2
