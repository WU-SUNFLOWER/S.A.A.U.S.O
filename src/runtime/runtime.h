// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_H_
#define SAAUSO_RUNTIME_RUNTIME_H_

#include <string_view>

#include "src/handles/handles.h"

namespace saauso::internal {

class Klass;
class PyObject;
class PyList;
class PyDict;
class PyString;
class PyTypeObject;
class PyCodeObject;
class PyTuple;
class PyModule;

bool Runtime_PyObjectIsTrue(Handle<PyObject> object);

bool Runtime_PyObjectIsTrue(Tagged<PyObject> object);

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable);

Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable);

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

int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value);

bool Runtime_IsPyObjectCallable(Tagged<PyObject> object);

Handle<PyTypeObject> Runtime_CreatePythonClass(Handle<PyString> class_name,
                                               Handle<PyDict> class_properties,
                                               Handle<PyList> supers);

Handle<PyObject> Runtime_FindPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name);

Handle<PyObject> Runtime_FindPropertyInKlassMro(Tagged<Klass> klass,
                                                Handle<PyObject> prop_name);

// 执行一个 PyCodeObject，并在指定 locals/globals 环境中运行。
// - locals/globals 必须是非空的 dict。
// - 若 globals 中缺少 __builtins__，则自动注入 builtins
// dict，保证内建符号可用。
// - 返回值为 code object 执行完成后的返回值（对 exec 来说通常会被上层忽略）。
Handle<PyObject> Runtime_ExecutePyCodeObject(Handle<PyCodeObject> code,
                                             Handle<PyDict> locals,
                                             Handle<PyDict> globals);

// 编译并执行一段 Python 源码，并在指定 locals/globals 环境中运行。
// - locals/globals 必须是非空的 dict。
// - 若 globals 中缺少 __builtins__，则自动注入 builtins dict。
// - 返回值为源码执行完成后的返回值（对 exec 来说通常会被上层忽略）。
Handle<PyObject> Runtime_ExecutePythonSourceCode(Handle<PyString> source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals);

// 编译并执行一段 Python 源码，并在指定 locals/globals 环境中运行。
// 语义与 Runtime_ExecutePythonSourceCode(PyString, ...) 一致。
Handle<PyObject> Runtime_ExecutePythonSourceCode(std::string_view source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_H_
