// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/stable-merge-sort.h"

#include <algorithm>
#include <vector>

namespace saauso::internal {

namespace {

inline int64_t Min(int64_t a, int64_t b) {
  return a < b ? a : b;
}

}  // namespace

void StableMergeSort::Sort(int64_t* data,
                           int64_t length,
                           LessFunc less,
                           void* context) {
  if (length <= 1) {
    return;
  }

  std::vector<int64_t> buffer(static_cast<size_t>(length));

  // 采用自底向上的迭代归并排序：每一轮把宽度为 width 的有序段两两归并成
  // 宽度为 2*width 的有序段，并使用双缓冲在 src/dst 间交换角色。
  int64_t* src = data;
  int64_t* dst = buffer.data();

  for (int64_t width = 1; width < length; width <<= 1) {
    for (int64_t left = 0; left < length; left += (width << 1)) {
      int64_t mid = Min(left + width, length);
      int64_t right = Min(left + (width << 1), length);

      int64_t i = left;
      int64_t j = mid;
      int64_t k = left;

      while (i < mid && j < right) {
        // 稳定性来源：当 src[j] 并不严格小于 src[i] 时，优先取左侧元素。
        if (less(src[j], src[i], context)) {
          dst[k++] = src[j++];
        } else {
          dst[k++] = src[i++];
        }
      }

      while (i < mid) {
        dst[k++] = src[i++];
      }

      while (j < right) {
        dst[k++] = src[j++];
      }
    }

    std::swap(src, dst);
  }

  // 若最终结果落在 buffer 中，则拷贝回 data。
  if (src != data) {
    std::copy(src, src + length, data);
  }
}

}  // namespace saauso::internal
