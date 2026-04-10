// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_EXCEPTION_SUPPORT_H_
#define SAAUSO_API_API_EXCEPTION_SUPPORT_H_

#include "src/api/api-handle-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"

namespace saauso {
namespace api {

bool CapturePendingException(i::Isolate* isolate);
Maybe<void> CapturePendingExceptionAndReturnNothing(i::Isolate* isolate);

template <typename T>
MaybeLocal<T> CapturePendingExceptionAndReturnEmptyLocal(i::Isolate* isolate) {
  CapturePendingException(isolate);
  return MaybeLocal<T>();
}

inline bool ToHandleOrCapturePendingException(i::Isolate* isolate,
                                              i::MaybeHandle<i::PyObject> value,
                                              i::Handle<i::PyObject>* out) {
  if (value.ToHandle(out)) {
    return true;
  }
  CapturePendingException(isolate);
  return false;
}

template <typename T>
MaybeLocal<T> ToLocalOrCapturePendingException(
    i::Isolate* isolate,
    i::EscapableHandleScope& handle_scope,
    i::MaybeHandle<i::PyObject> value) {
  i::Handle<i::PyObject> result;
  if (!ToHandleOrCapturePendingException(isolate, value, &result)) {
    return MaybeLocal<T>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return api::Utils::ToLocal<T>(escaped);
}

}  // namespace api
}  // namespace saauso

#endif
