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
  EXPECT_EQ(StringSearch::IndexOfSubstring("abc", ""), 0);
  EXPECT_EQ(StringSearch::IndexOfSubstring("", ""), 0);
}

TEST(StringSearchTest, LastIndexOfEmptyPattern) {
  EXPECT_EQ(StringSearch::LastIndexOfSubstring("abc", ""), 3);
  EXPECT_EQ(StringSearch::LastIndexOfSubstring("", ""), 0);
}

TEST(StringSearchTest, PatternLongerThanSubject) {
  EXPECT_EQ(StringSearch::IndexOfSubstring("abc", "abcd"),
            StringSearch::kNotFound);
  EXPECT_EQ(StringSearch::IndexOfSubstring("", "a"), StringSearch::kNotFound);
  EXPECT_EQ(StringSearch::LastIndexOfSubstring("abc", "abcd"),
            StringSearch::kNotFound);
}

TEST(StringSearchTest, FindsSmallPatternNaivePath) {
  EXPECT_EQ(StringSearch::IndexOfSubstring("abcabc", "cab"), 2);
  EXPECT_EQ(StringSearch::IndexOfSubstring("aaaaa", "aaa"), 0);
  EXPECT_EQ(StringSearch::IndexOfSubstring("aaaaa", "aaaaaa"),
            StringSearch::kNotFound);

  EXPECT_EQ(StringSearch::LastIndexOfSubstring("abcabc", "cab"), 2);
  EXPECT_EQ(StringSearch::LastIndexOfSubstring("aaaaa", "aa"), 3);
}

TEST(StringSearchTest, FindsLargePatternBoyerMoorePath) {
  EXPECT_EQ(StringSearch::IndexOfSubstring("0123456789abcdef", "56789abc"), 5);
  EXPECT_EQ(StringSearch::IndexOfSubstring("0123456789abcdef", "89abcdef"), 8);
  EXPECT_EQ(StringSearch::IndexOfSubstring("0123456789abcdef", "notfound!"),
            StringSearch::kNotFound);
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

  EXPECT_EQ(StringSearch::IndexOfSubstring(std::string_view(subject),
                                           std::string_view(pattern)),
            1);

  EXPECT_EQ(StringSearch::LastIndexOfSubstring(std::string_view(subject),
                                               std::string_view(pattern)),
            1);
}

}  // namespace saauso::internal
