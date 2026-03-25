// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_
#define SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class Isolate;

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
MaybeHandle<PyTypeObject> Runtime_CreatePythonClass(
    Isolate* isolate,
    Handle<PyString> class_name,
    Handle<PyDict> class_properties,
    Handle<PyList> supers);

// 判断 object 是否是某个 type 的实例。
// - 该函数仅依赖对象的 klass mro，不做 Python 层参数校验。
// - 入参应由上层（builtin/解释器语义层）保证为合法的 type object。
Maybe<bool> Runtime_IsInstanceOfTypeObject(Handle<PyObject> object,
                                           Handle<PyTypeObject> type_object);

// 判断 derive_type_object 是否是 super_type_object 的派生类型（含自身）。
// - 基于 derive_type_object 的 klass mro 进行判定。
// - 命中返回true，未命中返回false，比较过程中出现异常返回empty。
Maybe<bool> Runtime_IsSubtype(Handle<PyTypeObject> derive_type_object,
                              Handle<PyTypeObject> super_type_object);

// 判断 derive_klass 是否是 super_klass 的派生类型（含自身）。
// - 基于 derive_klass 的 mro 进行判定。
// - 命中返回true，未命中返回false。
Maybe<bool> Runtime_IsSubtype(Tagged<Klass> derive_klass,
                              Tagged<Klass> super_klass);

// 沿着实例对象的 type mro 查找属性（Lookup 语义）。
// - 命中通过out_prop_val输出属性值，并且返回true
// - 未命中通过out_prop_val输出null，并且返回false
// - 查询过程中出现异常，out_prop_val输出null，并且返回empty
Maybe<bool> Runtime_LookupPropertyInInstanceTypeMro(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> prop_name,
    Handle<PyObject>& out_prop_val);

// 沿着 klass 的 mro 查找属性（Lookup 语义）。
// - 命中通过out_prop_val输出属性值，并且返回true
// - 未命中通过out_prop_val输出null，并且返回false
// - 查询过程中出现异常，out_prop_val输出null，并且返回empty
Maybe<bool> Runtime_LookupPropertyInKlassMro(Isolate* isolate,
                                             Tagged<Klass> klass,
                                             Handle<PyObject> prop_name,
                                             Handle<PyObject>& out_prop_val);

// 沿着 klass 的 mro 获取属性（Get 语义）。
// - 命中返回属性值
// - 未命中抛出 AttributeError，并返回empty
// - 查询过程中出现异常，透传异常并返回empty
MaybeHandle<PyObject> Runtime_GetPropertyInKlassMro(Isolate* isolate,
                                                    Tagged<Klass> klass,
                                                    Handle<PyObject> prop_name);

// 沿着实例对象的 type mro 获取属性（Get 语义）。
// - 命中返回属性值
// - 未命中抛出 AttributeError，并返回empty
// - 查询过程中出现异常，透传异常并返回empty
MaybeHandle<PyObject> Runtime_GetPropertyInInstanceTypeMro(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> prop_name);

// 沿着对象的 type mro 查找某个 magic method 并立即调用。
// - func_name 必须为非空的 str。
// - args/kwargs 允许为 null。
// - 若未找到对应方法，则抛出 TypeError。
MaybeHandle<PyObject> Runtime_InvokeMagicOperationMethod(
    Isolate* isolate,
    Handle<PyObject> object,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs,
    Handle<PyObject> func_name);

MaybeHandle<PyObject> Runtime_NewObject(Isolate* isolate,
                                        Handle<PyTypeObject> type_object,
                                        Handle<PyObject> args,
                                        Handle<PyObject> kwargs);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_REFLECTION_H_
