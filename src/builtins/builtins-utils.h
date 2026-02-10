// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_BUILTINS_BUILTINS_UTILS_H_
#define SRC_BUILTINS_BUILTINS_UTILS_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyTuple;
class PyDict;

#define BUILTIN_FUNC_NAME(name) Builtin_##name

#define BUILTIN(name)                       \
  Handle<PyObject> BUILTIN_FUNC_NAME(name)( \
      Handle<PyObject> host, Handle<PyTuple> args, Handle<PyDict> kwargs)

}  // namespace saauso::internal

#endif  // SRC_BUILTINS_BUILTINS_UTILS_H_
