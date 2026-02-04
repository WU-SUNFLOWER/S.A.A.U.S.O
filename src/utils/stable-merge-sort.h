// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_STABLE_MERGE_SORT_H_
#define SAAUSO_UTILS_STABLE_MERGE_SORT_H_

#include <cstdint>

namespace saauso::internal {

// 稳定归并排序工具。
//
// 该实现提供可注入的比较回调，并保证稳定性：当两个元素“比较相等”时，
// 其相对顺序保持不变。
class StableMergeSort {
 public:
  using LessFunc = bool (*)(int64_t a, int64_t b, void* context);

  // 对 data 指向的数组进行稳定排序。
  // - less(a, b, context) 返回 true 表示 a 严格小于 b。
  // - context 原样透传给比较回调，便于携带额外比较所需的数据。
  // - length <= 1 时不做任何操作。
  static void Sort(int64_t* data, int64_t length, LessFunc less, void* context);
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_STABLE_MERGE_SORT_H_
