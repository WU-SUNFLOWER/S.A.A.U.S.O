// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <string>
#include <string_view>

#include "src/utils/string-search.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

TEST(StringSearchTest, EmptyPattern) {
  EXPECT_EQ(IndexOfSubstring("abc", ""), 0);
  EXPECT_EQ(IndexOfSubstring("", ""), 0);
}

TEST(StringSearchTest, PatternLongerThanSubject) {
  EXPECT_EQ(IndexOfSubstring("abc", "abcd"), -1);
  EXPECT_EQ(IndexOfSubstring("", "a"), -1);
}

TEST(StringSearchTest, FindsSmallPatternNaivePath) {
  EXPECT_EQ(IndexOfSubstring("abcabc", "cab"), 2);
  EXPECT_EQ(IndexOfSubstring("aaaaa", "aaa"), 0);
  EXPECT_EQ(IndexOfSubstring("aaaaa", "aaaaaa"), -1);
}

TEST(StringSearchTest, FindsLargePatternBoyerMoorePath) {
  EXPECT_EQ(IndexOfSubstring("0123456789abcdef", "56789abc"), 5);
  EXPECT_EQ(IndexOfSubstring("0123456789abcdef", "89abcdef"), 8);
  EXPECT_EQ(IndexOfSubstring("0123456789abcdef", "notfound!"), -1);
}

TEST(StringSearchTest, BinarySafe) {
  std::string subject;
  subject.push_back('a');
  subject.push_back('\0');
  subject.push_back('b');
  subject.push_back('\0');
  subject.push_back('c');

  std::string pattern;
  pattern.push_back('\0');
  pattern.push_back('b');
  pattern.push_back('\0');

  EXPECT_EQ(IndexOfSubstring(std::string_view(subject), std::string_view(pattern)),
            1);
}

}  // namespace saauso::internal

