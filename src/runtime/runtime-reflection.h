// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_
#define SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;
class PyTypeObject;
class PyString;
class PyList;
class PyDict;
class PyTuple;
class Klass;

// 创建一个新的 Python 类（返回其 type object）。
// - class_name/class_properties/supers 均必须为非空。
// - 该函数会创建新的 Klass 并注册进当前 Isolate。
Handle<PyTypeObject> Runtime_CreatePythonClass(Handle<PyString> class_name,
                                               Handle<PyDict> class_properties,
                                               Handle<PyList> supers);

// 判断 object 是否是某个 type 的实例。
// - 该函数仅依赖对象的 klass mro，不做 Python 层参数校验。
// - 入参应由上层（builtin/解释器语义层）保证为合法的 type object。
bool Runtime_IsInstanceOfTypeObject(Handle<PyObject> object,
                                    Handle<PyTypeObject> type_object);

// 沿着 instance 的 type mro 查找属性。
// - 命中返回属性值；未命中返回 null。
// - 该函数不会抛出 Python 异常；返回 null 仅表示缺失（not found）。
Handle<PyObject> Runtime_FindPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name);

// 沿着 klass 的 mro 查找属性。
// - 命中返回属性值；未命中返回 null。
// - 该函数不会抛出 Python 异常；返回 null 仅表示缺失（not found）。
Handle<PyObject> Runtime_FindPropertyInKlassMro(Tagged<Klass> klass,
                                                Handle<PyObject> prop_name);

// 沿着对象的 type mro 查找某个 magic method 并立即调用。
// - func_name 必须为非空的 str。
// - args/kwargs 允许为 null。
// - 若未找到对应方法，则抛出 TypeError。
MaybeHandle<PyObject> Runtime_InvokeMagicOperationMethod(
    Handle<PyObject> object,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs,
    Handle<PyObject> func_name);

MaybeHandle<PyObject> Runtime_NewType(Handle<PyObject> args,
                                      Handle<PyObject> kwargs);

MaybeHandle<PyObject> Runtime_NewObject(Handle<PyTypeObject> type_object,
                                        Handle<PyObject> args,
                                        Handle<PyObject> kwargs);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_
