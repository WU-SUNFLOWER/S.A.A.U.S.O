// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_ISOLATE_H_
#define INCLUDE_SAAUSO_ISOLATE_H_

#include "saauso-exception.h"
#include "saauso-value.h"

namespace saauso {

struct IsolateCreateParams {};

class Isolate {
 public:
  static Isolate* New(
      const IsolateCreateParams& params = IsolateCreateParams());
  void Dispose();
  void ThrowException(Local<Value> exception);
  size_t ValueRegistrySizeForTesting() const;

  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

 private:
  Isolate() = default;

  void* internal_isolate_{nullptr};
  void* entered_contexts_{nullptr};
  TryCatch* try_catch_top_{nullptr};

  friend class Context;
  friend class TryCatch;
  friend struct ApiAccess;
};

}  // namespace saauso

#endif
