// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_TRUTHINESS_H_
#define SSAUSO_RUNTIME_RUNTIME_TRUTHINESS_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;

// 判断一个 Python 对象的真值（truthiness）。
// - 支持常见内建类型的快速路径；对未覆盖类型当前回退为 true。
bool Runtime_PyObjectIsTrue(Handle<PyObject> object);

// 判断一个 Python 对象的真值（truthiness）。
// - 参数允许为 null；null 会被按 false 处理（与上层调用点习惯对齐）。
bool Runtime_PyObjectIsTrue(Tagged<PyObject> object);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_TRUTHINESS_H_
