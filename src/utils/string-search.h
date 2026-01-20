// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_STRING_SEARCH_H_
#define SAAUSO_UTILS_STRING_SEARCH_H_

#include <cstdint>
#include <string_view>

namespace saauso::internal {

inline constexpr int64_t kBMMinPatternLength = 8;

int64_t IndexOfSubstring(std::string_view subject, std::string_view pattern);

inline bool ContainsSubstring(std::string_view subject, std::string_view pattern) {
  return IndexOfSubstring(subject, pattern) != -1;
}

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_STRING_SEARCH_H_

