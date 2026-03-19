// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_INL_H_
#define SAAUSO_API_API_INL_H_

#include "include/saauso-embedder.h"
#include "src/execution/isolate.h"

namespace saauso {

struct ApiAccess {
  static internal::Isolate* UnwrapIsolate(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<internal::Isolate*>(isolate->internal_isolate_);
  }

  static const internal::Isolate* UnwrapIsolate(
      const saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<const internal::Isolate*>(
        isolate->internal_isolate_);
  }
};

}  // namespace saauso

#endif  // SAAUSO_API_API_INL_H_
