// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_STABLE_MERGE_SORT_H_
#define SAAUSO_UTILS_STABLE_MERGE_SORT_H_

#include <cstdint>

namespace saauso::internal {

class StableMergeSort {
 public:
  using LessFunc = bool (*)(int64_t a, int64_t b, void* context);

  static void Sort(int64_t* data, int64_t length, LessFunc less, void* context);
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_STABLE_MERGE_SORT_H_

