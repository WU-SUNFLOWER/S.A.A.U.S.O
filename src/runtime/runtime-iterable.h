// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_
#define SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyList;
class PyTuple;

// 将一个可迭代对象展开并追加到 list 末尾。
// - list 必须为非空。
// - iteratable 必须可被 Iter(...)；否则会在下层 fail-fast。
void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable);

// 将一个可迭代对象转换为 tuple。
// - iterable 必须可被 Iter(...)；否则会在下层 fail-fast。
Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_
