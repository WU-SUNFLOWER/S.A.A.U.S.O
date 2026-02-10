// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_
#define SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

Handle<PyObject> Native_Print(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs);

Handle<PyObject> Native_Len(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs);

Handle<PyObject> Native_IsInstance(Handle<PyObject> host,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs);

Handle<PyObject> Native_BuildTypeObject(Handle<PyObject> host,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs);

Handle<PyObject> Native_Sysgc(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs);

// 执行 Python 源码或 code object（内建 exec）。
// - 位置参数形态：exec(obj) / exec(obj, globals) / exec(obj, globals, locals)。
// - 关键字参数：仅允许 globals/locals；其它关键字直接报错。
// - globals/locals 省略或传 None 时，会默认使用“当前执行帧”的环境字典；
//   当当前帧 locals 不存在时，会回退为 globals。
// - 返回值始终为 None（对齐 CPython 的 exec）。
Handle<PyObject> Native_Exec(Handle<PyObject> host,
                             Handle<PyTuple> args,
                             Handle<PyDict> kwargs);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_
