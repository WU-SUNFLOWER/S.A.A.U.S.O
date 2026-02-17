// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_H_
#define SAAUSO_RUNTIME_RUNTIME_H_

#include <cstdint>
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

inline constexpr std::string_view kDefaultSourceFilename = "<string>";

// 判断一个 Python 对象的真值（truthiness）。
// - 支持常见内建类型的快速路径；对未覆盖类型当前回退为 true。
bool Runtime_PyObjectIsTrue(Handle<PyObject> object);

// 判断一个 Python 对象的真值（truthiness）。
// - 参数允许为 null；null 会被按 false 处理（与上层调用点习惯对齐）。
bool Runtime_PyObjectIsTrue(Tagged<PyObject> object);

// 判断一个 Python 对象是否可调用（callable）。
// - 参数允许为 null；null 返回 false。
// - 该函数不会触发 Python 层的 __call__ 查找，仅检查已知可调用对象与 vtable
//   slot。
bool Runtime_IsPyObjectCallable(Tagged<PyObject> object);

// 将一个可迭代对象展开并追加到 list 末尾。
// - list 必须为非空。
// - iteratable 必须可被 Iter(...)；否则会在下层 fail-fast。
void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable);

// 将一个可迭代对象转换为 tuple。
// - iterable 必须可被 Iter(...)；否则会在下层 fail-fast。
Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable);

// 将一个 Python 字符串按指定分隔符拆分为 list。
// - sep_or_null 为 null 表示按空白拆分（行为对齐 CPython 的子集语义）。
// - sep_or_null 非 null 时必须为 str；空字符串分隔符会触发 ValueError 并
// fail-fast。
// - maxsplit < 0 表示不限次数；maxsplit == 0 返回至多一个元素（保留现有行为）。
Handle<PyList> Runtime_PyStringSplit(Handle<PyString> str,
                                     Handle<PyObject> sep_or_null,
                                     int64_t maxsplit);

// 将一个可迭代对象按 Python 语义用分隔符连接为 str。
// - iterable 必须为非空且可被展开；否则会在下层 fail-fast。
// - iterable 的每个元素必须为 str，否则会打印 TypeError 并 fail-fast。
Handle<PyString> Runtime_PyStringJoin(Handle<PyString> str,
                                      Handle<PyObject> iterable);

// 按 Python 语义将任意对象转换为 str。
// - 支持 str/int/float/bool/None 的快速路径。
// - 其它对象会尝试调用 __str__；若找不到 __str__，回退为 "<object at 0x...>"。
Handle<PyString> Runtime_NewStr(Handle<PyObject> value);

// 沿着对象的 type mro 查找某个 magic method 并立即调用。
// - func_name 必须为非空的 str。
// - args/kwargs 允许为 null。
// - 若未找到对应方法，则打印错误信息并 fail-fast。
Handle<PyObject> Runtime_InvokeMagicOperationMethod(Handle<PyObject> object,
                                                    Handle<PyTuple> args,
                                                    Handle<PyDict> kwargs,
                                                    Handle<PyObject> func_name);

Handle<PyObject> Runtime_NewType(Handle<PyObject> args,
                                 Handle<PyObject> kwargs);

Handle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                 Handle<PyObject> kwargs);

Handle<PyObject> Runtime_NewSmi(Handle<PyObject> args, Handle<PyObject> kwargs);

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

// 将 int/bool 等“可解释为整数”的对象解码为 int64_t。
// - 当前仅支持 Smi 与 bool；不支持时 fail-fast。
int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value);

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
Handle<PyObject> Runtime_FindPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name);

// 沿着 klass 的 mro 查找属性。
// - 命中返回属性值；未命中返回 null。
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
Handle<PyObject> Runtime_ExecutePythonSourceCode(
    Handle<PyString> source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename = kDefaultSourceFilename);

// 编译并执行一段 Python 源码，并在指定 locals/globals 环境中运行。
// 语义与 Runtime_ExecutePythonSourceCode(PyString, ...) 一致。
Handle<PyObject> Runtime_ExecutePythonSourceCode(
    std::string_view source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename = kDefaultSourceFilename);

Handle<PyObject> Runtime_ExecutePythonPycFile(std::string_view filename,
                                              Handle<PyDict> locals,
                                              Handle<PyDict> globals);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_H_
