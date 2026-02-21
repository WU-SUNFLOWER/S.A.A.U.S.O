// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;

MaybeHandle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                      Handle<PyObject> kwargs);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_
