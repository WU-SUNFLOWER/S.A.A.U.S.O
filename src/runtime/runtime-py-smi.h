// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PySmi;
class PyFloat;
class PyString;

MaybeHandle<PySmi> Runtime_PyStringToSmi(Tagged<PyString> py_string);

MaybeHandle<PySmi> Runtime_PyFloatToSmi(Tagged<PyFloat> py_float);

// 按 Python 语义构造 int 对象。
// - 失败时返回 empty，并保证已设置 pending exception。
MaybeHandle<PySmi> Runtime_NewSmi(Handle<PyObject> args,
                                  Handle<PyObject> kwargs);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_
