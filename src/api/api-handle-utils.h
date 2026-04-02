// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_HANDLE_UTILS_H_
#define SAAUSO_API_API_HANDLE_UTILS_H_

#include "include/saauso-local-handle.h"
#include "src/handles/handles.h"
#include "src/objects/py-object.h"

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

}  // namespace api
}  // namespace saauso

#endif
