// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_SUPPORT_H_
#define SAAUSO_API_API_SUPPORT_H_

#include <cassert>

#include "include/saauso-local-handle.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-tuple.h"

namespace saauso {
namespace api {

class Utils {
 public:
  template <typename T>
  static inline i::Handle<i::PyObject> OpenHandle(Local<T> local) {
    return OpenHandle(*local);
  }

  template <typename T>
  static inline i::Handle<i::PyObject> OpenHandle(const T* value) {
    if (value == nullptr) {
      return i::Handle<i::PyObject>::null();
    }
    return i::Handle<i::PyObject>(
        reinterpret_cast<i::Address*>(const_cast<T*>(value)));
  }

  template <typename T>
  static inline Local<T> ToLocal(i::Handle<i::PyObject> handle) {
    return Local<T>::FromSlot(handle.location());
  }
};

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

//////////////////////////////////////////////////////////////////////////
// Exception API 实现相关支持

bool CapturePendingException(i::Isolate* isolate);

//////////////////////////////////////////////////////////////////////////
// Function Callback API 实现相关支持

struct FunctionCallbackInfoImpl {
  i::Isolate* isolate{nullptr};
  i::Handle<i::PyObject> receiver;
  i::Handle<i::PyTuple> args;
  Local<Value> return_value;
};

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict> kwargs,
    i::Handle<i::PyObject> closure_data);

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_SUPPORT_H_
