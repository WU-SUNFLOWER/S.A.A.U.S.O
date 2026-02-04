// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "src/utils/number-conversion.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

std::string DoubleToString(double n) {
  std::array<char, 64> buf{};
  DoubleToStringView(n, std::string_view(buf.data(), buf.size()));
  return std::string(buf.data());
}

std::string Int64ToString(int64_t n) {
  std::array<char, 64> buf{};
  Int64ToStringView(n, std::string_view(buf.data(), buf.size()));
  return std::string(buf.data());
}

}  // namespace

TEST(NumberConversionTest, StringToIntRejectsGarbage) {
  EXPECT_FALSE(StringToInt("").has_value());
  EXPECT_FALSE(StringToInt("123Hello").has_value());
  EXPECT_FALSE(StringToInt("Hello World").has_value());
  EXPECT_FALSE(StringToInt("1__2").has_value());
  EXPECT_FALSE(StringToInt("_12").has_value());
  EXPECT_FALSE(StringToInt("12_").has_value());
}

TEST(NumberConversionTest, StringToIntAcceptsWhitespaceSignUnderscore) {
  EXPECT_EQ(*StringToInt("0"), 0);
  EXPECT_EQ(*StringToInt("  42 "), 42);
  EXPECT_EQ(*StringToInt("+42"), 42);
  EXPECT_EQ(*StringToInt("-42"), -42);
  EXPECT_EQ(*StringToInt("1_000_000"), 1000000);
}

TEST(NumberConversionTest, StringToDoubleBasics) {
  EXPECT_FALSE(StringToDouble("").has_value());
  EXPECT_FALSE(StringToDouble("1.2.3").has_value());
  EXPECT_FALSE(StringToDouble("3.14pi").has_value());

  EXPECT_DOUBLE_EQ(*StringToDouble("3.14"), 3.14);
  EXPECT_DOUBLE_EQ(*StringToDouble("  -1.25 "), -1.25);
  EXPECT_DOUBLE_EQ(*StringToDouble("1_0.5_0"), 10.50);
  EXPECT_DOUBLE_EQ(*StringToDouble("1e-05"), 1e-05);
  EXPECT_DOUBLE_EQ(*StringToDouble("1e+06"), 1e6);
}

TEST(NumberConversionTest, StringToDoubleSpecialTokens) {
  auto nan1 = StringToDouble("nan");
  ASSERT_TRUE(nan1.has_value());
  EXPECT_TRUE(std::isnan(*nan1));

  EXPECT_TRUE(std::isnan(*StringToDouble("-NaN")));
  EXPECT_EQ(*StringToDouble("inf"), std::numeric_limits<double>::infinity());
  EXPECT_EQ(*StringToDouble("+Infinity"), std::numeric_limits<double>::infinity());
  EXPECT_EQ(*StringToDouble("-inf"), -std::numeric_limits<double>::infinity());
}

TEST(NumberConversionTest, Int64ToStringViewWorks) {
  EXPECT_EQ(Int64ToString(0), "0");
  EXPECT_EQ(Int64ToString(42), "42");
  EXPECT_EQ(Int64ToString(-42), "-42");
  EXPECT_EQ(Int64ToString(std::numeric_limits<int64_t>::min()),
            "-9223372036854775808");
}

TEST(NumberConversionTest, DoubleToStringViewSpecialCases) {
  EXPECT_EQ(DoubleToString(std::numeric_limits<double>::quiet_NaN()), "nan");
  EXPECT_EQ(DoubleToString(std::numeric_limits<double>::infinity()), "inf");
  EXPECT_EQ(DoubleToString(-std::numeric_limits<double>::infinity()), "-inf");
  EXPECT_EQ(DoubleToString(-0.0), "-0.0");
  EXPECT_EQ(DoubleToString(0.0), "0.0");
}

TEST(NumberConversionTest, DoubleToStringViewMatchesPythonThresholds) {
  EXPECT_EQ(DoubleToString(1.0), "1.0");
  EXPECT_EQ(DoubleToString(1000000.0), "1000000.0");
  EXPECT_EQ(DoubleToString(1e-5), "1e-05");
  EXPECT_EQ(DoubleToString(1e-4), "0.0001");
  EXPECT_EQ(DoubleToString(1e16), "1e+16");
}

}  // namespace saauso::internal

