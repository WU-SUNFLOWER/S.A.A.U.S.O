// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_NATIVE_FUNCTION_H_
#define INCLUDE_SAAUSO_NATIVE_FUNCTION_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;
class PyTuple;
class PyDict;

enum class NativeFunctionCallKind : uint8_t {
  kPlainFunction = 0,
  kMethodDescriptor = 1,
};

using NativeFuncPointer = MaybeHandle<PyObject> (*)(Isolate* isolate,
                                                    Handle<PyObject> receiver,
                                                    Handle<PyTuple> args,
                                                    Handle<PyDict> kwargs);

}  // namespace saauso::internal

#endif  // INCLUDE_SAAUSO_NATIVE_FUNCTION_H_
