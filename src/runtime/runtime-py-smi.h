// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_

#include "src/handles/handles.h"

namespace saauso::internal {

Handle<PyObject> Runtime_NewSmi(Handle<PyObject> args, Handle<PyObject> kwargs);

}

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_SMI_H_
