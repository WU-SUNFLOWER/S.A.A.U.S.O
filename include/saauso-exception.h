// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_EXCEPTION_H_
#define INCLUDE_SAAUSO_EXCEPTION_H_

#include "saauso-local-handle.h"

namespace saauso {

class String;

class Exception {
 public:
  static Local<Value> TypeError(Local<String> msg);
  static Local<Value> RuntimeError(Local<String> msg);
};

class TryCatch {
 public:
  explicit TryCatch(Isolate* isolate);
  TryCatch(const TryCatch&) = delete;
  TryCatch& operator=(const TryCatch&) = delete;
  ~TryCatch();

  bool HasCaught() const;
  void Reset();
  Local<Value> Exception() const;

 private:
  Isolate* isolate_{nullptr};
  TryCatch* previous_{nullptr};
  Local<Value> exception_;

  friend struct ApiAccess;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EXCEPTION_H_
