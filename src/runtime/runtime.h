// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"

#ifndef SAAUSO_RUNTIME_RUNTIME_H_
#define SAAUSO_RUNTIME_RUNTIME_H_

namespace saauso::internal {

class PyObject;

bool Runtime_PyObjectIsTrue(Handle<PyObject> object);
bool Runtime_PyObjectIsTrue(Tagged<PyObject> object);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_H_
