// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_STRING_SEARCH_H_
#define SAAUSO_UTILS_STRING_SEARCH_H_

#include <cstdint>
#include <string_view>

namespace saauso::internal {

// 子串查找工具。
//
// 该模块属于纯工具层（src/utils），不依赖虚拟机上层能力。对短模式串使用朴素
// 算法以降低常数开销；对较长模式串使用 Boyer-Moore 以减少平均比较次数。
class StringSearch {
 public:
  // Boyer-Moore 的最小启用阈值（小于该长度时优先走朴素搜索）。
  static constexpr int64_t kBMMinPatternLength = 8;
  // 未找到子串时返回的索引值。
  static constexpr int kNotFound = -1;

  // 在 subject 中查找 pattern 的第一次出现位置。
  // - pattern 为空时返回 0。
  // - 未找到时返回 kNotFound。
  static int64_t IndexOfSubstring(std::string_view subject,
                                  std::string_view pattern);

  // 判断 subject 是否包含 pattern。
  static bool ContainsSubstring(std::string_view subject,
                                std::string_view pattern) {
    return IndexOfSubstring(subject, pattern) != kNotFound;
  }
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_STRING_SEARCH_H_
