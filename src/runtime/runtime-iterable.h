// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_
#define SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class Isolate;

class PyList;
class PyTuple;

// 将一个可迭代对象展开并追加到 list 末尾。
// - list 必须为非空。
// - iteratable 必须可被 Iter(...)。
// - 成功时返回 None，同时 list 被成功拓展。
// - 失败时返回 empty，并保证已设置 pending exception。
MaybeHandle<PyObject> Runtime_ExtendListByItratableObject(
    Isolate* isolate,
    Handle<PyList> list,
    Handle<PyObject> iteratable);

// 将一个可迭代对象转换为 tuple。
// - iterable 必须可被 Iter(...)。
// - 失败时返回 empty，并保证已设置 pending exception。
MaybeHandle<PyTuple> Runtime_UnpackIterableObjectToTuple(
    Isolate* isolate,
    Handle<PyObject> iterable);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_ITERABLE_H_
