// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_ISOLATE_UTILS_H_
#define SAAUSO_API_API_ISOLATE_UTILS_H_

#include <cassert>

#include "src/execution/isolate.h"

namespace saauso {

class Isolate;

namespace api {

inline i::Isolate* RequireCurrentIsolate() {
  i::Isolate* isolate = i::Isolate::Current();
  assert(isolate != nullptr);
  return isolate;
}

inline i::Isolate* RequireExplicitIsolate(Isolate* isolate) {
  i::Isolate* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(internal_isolate != nullptr);
  assert(internal_isolate == i::Isolate::Current());
  return internal_isolate;
}

}  // namespace api
}  // namespace saauso

#endif
