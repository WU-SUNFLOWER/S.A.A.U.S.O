// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_INTRINSICS_H_
#define SSAUSO_RUNTIME_RUNTIME_INTRINSICS_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyTuple;
class PyDict;

// intrinsic：将一个 list 转换为 tuple。
// - object 必须是 list，否则 fail-fast。
Handle<PyTuple> Runtime_IntrinsicListToTuple(Handle<PyObject> list);

// 导入某个模块名下的所有子模块到解释器栈帧的locals
// 注意：
// 在Python中，假设包package当中的__init__.py没有显
// 式设置__all__，并且package名下的所有子包都从没有被显
// 式导入，那么执行`from package import *`后，虚拟机
// 实际上不会主动查找package目录下的模块文件或子包目录。
// 因此用户也无法直接使用package名下的子模块。
void Runtime_IntrinsicImportStar(Handle<PyObject> module,
                                 Handle<PyDict> locals);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_INTRINSICS_H_
