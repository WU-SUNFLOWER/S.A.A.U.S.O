// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/builtin-bootstrapper.h"

#include <string>

#include "src/builtins/builtins-definitions.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

struct BuiltinTypeEntry {
  const char* name;
  Handle<PyObject> type_object;
};

struct BuiltinOddballEntry {
  const char* name;
  Tagged<PyObject> value;
};

struct BuiltinFunctionEntry {
  Handle<PyString> name;
  NativeFuncPointer func;
};

// BaseException.__str__ 的最小实现：返回 message 字段。
// 该实现用于提升 MVP 阶段的可用性，使用户能够通过 str(e) 获取错误原因。
Handle<PyObject> Native_BaseExceptionStr(Handle<PyObject> host,
                                         Handle<PyTuple> args,
                                         Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowTypeError(
        "BaseException.__str__() takes no keyword arguments");
    return Handle<PyObject>::null();
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowTypeError("BaseException.__str__() takes no arguments");
    return Handle<PyObject>::null();
  }

  if (host.is_null()) [[unlikely]] {
    Runtime_ThrowTypeError(
        "BaseException.__str__() expects a non-null receiver");
    return Handle<PyObject>::null();
  }

  Handle<PyDict> properties = PyObject::GetProperties(host);
  if (!properties.is_null()) {
    Handle<PyObject> message = properties->Get(ST(message));
    if (!message.is_null() && IsPyString(*message)) {
      return scope.Escape(Handle<PyString>::cast(message));
    }
  }

  return scope.Escape(PyString::NewInstance(""));
}

// BaseException.__repr__ 的最小实现：返回 "<TypeName: message>"（message
// 为空则省略）。 该实现主要用于调试与单测断言，MVP 阶段不追求完全对齐 CPython
// repr。
Handle<PyObject> Native_BaseExceptionRepr(Handle<PyObject> host,
                                          Handle<PyTuple> args,
                                          Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowTypeError(
        "BaseException.__repr__() takes no keyword arguments");
    return Handle<PyObject>::null();
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowTypeError("BaseException.__repr__() takes no arguments");
    return Handle<PyObject>::null();
  }

  if (host.is_null()) [[unlikely]] {
    Runtime_ThrowTypeError(
        "BaseException.__repr__() expects a non-null receiver");
    return Handle<PyObject>::null();
  }

  Handle<PyString> type_name = PyObject::GetKlass(host)->name();
  Handle<PyObject> message_obj = Native_BaseExceptionStr(host, args, kwargs);
  if (Isolate::Current()->interpreter()->HasPendingException()) {
    return Handle<PyObject>::null();
  }
  Handle<PyString> message = message_obj.is_null()
                                 ? Handle<PyString>::null()
                                 : Handle<PyString>::cast(message_obj);

  std::string buf;
  if (message.is_null() || message->IsEmpty()) {
    buf = "<";
    buf.append(type_name->buffer(), static_cast<size_t>(type_name->length()));
    buf.append(">");
  } else {
    buf = "<";
    buf.append(type_name->buffer(), static_cast<size_t>(type_name->length()));
    buf.append(": ");
    buf.append(message->buffer(), static_cast<size_t>(message->length()));
    buf.append(">");
  }
  return scope.Escape(
      PyString::NewInstance(buf.c_str(), static_cast<int64_t>(buf.size())));
}

}  // namespace

///////////////////////////////////////////////////////////////////

BuiltinBootstrapper::BuiltinBootstrapper(Isolate* isolate)
    : isolate_(isolate) {}

Handle<PyDict> BuiltinBootstrapper::CreateBuiltins() {
  EscapableHandleScope scope;

  assert(!is_bootstrapped_);
  builtins_ = PyDict::NewInstance();

  InstallBuiltinTypes();
  InstallOddballs();
  InstallBuiltinFunctions();
  InstallBuiltinsSelfReference();
  InstallBuiltinExceptionTypes();

  is_bootstrapped_ = true;
  return scope.Escape(builtins_.Get());
}

void BuiltinBootstrapper::InstallBuiltinTypes() {
#define BUILTIN_TYPE_ENTRY(name, klass) \
  {name, klass::GetInstance()->type_object()},

#define BUILTIN_TYPE_LIST(V) \
  V("object", PyObjectKlass) \
  V("int", PySmiKlass)       \
  V("str", PyStringKlass)    \
  V("float", PyFloatKlass)   \
  V("list", PyListKlass)     \
  V("bool", PyBooleanKlass)  \
  V("dict", PyDictKlass)     \
  V("tuple", PyTupleKlass)   \
  V("type", PyTypeObjectKlass)

  const BuiltinTypeEntry entries[] = {BUILTIN_TYPE_LIST(BUILTIN_TYPE_ENTRY)};

#undef BUILTIN_TYPE_LIST
#undef BUILTIN_TYPE_ENTRY

  auto builtins_handle = builtins_.Get();
  for (const auto& entry : entries) {
    PyDict::Put(builtins_handle, PyString::NewInstance(entry.name),
                entry.type_object);
  }
}

void BuiltinBootstrapper::InstallOddballs() {
  // 注册解释器侧可见的单例对象。
  const BuiltinOddballEntry entries[] = {
      {"True", isolate_->py_true_object()},
      {"False", isolate_->py_false_object()},
      {"None", isolate_->py_none_object()},
  };

  auto builtins_handle = builtins_.Get();
  for (const auto& entry : entries) {
    PyDict::Put(builtins_handle, PyString::NewInstance(entry.name),
                handle(entry.value));
  }
}

void BuiltinBootstrapper::InstallBuiltinFunctions() {
#define BUILTIN_FUNC_ENTRY(func_name_in_string_table, func_cpp_symbol) \
  {ST(func_name_in_string_table), &BUILTIN_FUNC_NAME(func_cpp_symbol)},

#define BUILTIN_FUNC_LIST(V)           \
  V(func_print, Print)                 \
  V(func_len, Len)                     \
  V(func_isinstance, IsInstance)       \
  V(func_build_class, BuildTypeObject) \
  V(func_sysgc, Sysgc)                 \
  V(func_exec, Exec)

  const BuiltinFunctionEntry entries[] = {
      BUILTIN_FUNC_LIST(BUILTIN_FUNC_ENTRY)};

  auto builtins_handle = builtins_.Get();
  for (const auto& entry : entries) {
    PyDict::Put(builtins_handle, entry.name,
                PyFunction::NewInstance(entry.func, entry.name));
  }

#undef BUILTIN_FUNC_LIST
#undef BUILTIN_FUNC_ENTRY
}

void BuiltinBootstrapper::InstallBuiltinsSelfReference() {
  auto builtins_handle = builtins_.Get();
  PyDict::Put(builtins_handle, ST(builtins), builtins_handle);
}

void BuiltinBootstrapper::InstallBuiltinExceptionTypes() {
  // 先创建BaseException和Exception，确保后续代码中Exception可用
  InstallBuiltinBasicExceptionTypes();

  auto builtins_handle = builtins_.Get();
  auto exception_type = builtins_handle->Get(ST(exception));
  RegisterSimpleTypeToBuiltins(ST(type_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(value_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(name_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(attr_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(index_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(key_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(runtime_err), exception_type);
  RegisterSimpleTypeToBuiltins(ST(div_zero), exception_type);
  RegisterSimpleTypeToBuiltins(ST(stop_iter), exception_type);
}

void BuiltinBootstrapper::InstallBuiltinBasicExceptionTypes() {
  Handle<PyTypeObject> object_type =
      Handle<PyTypeObject>::cast(PyObjectKlass::GetInstance()->type_object());

  auto supers = PyList::NewInstance(1);
  supers->SetAndExtendLength(0, object_type);
  Handle<PyDict> base_exception_dict = PyDict::NewInstance();
  Handle<PyTypeObject> base_exception = Runtime_CreatePythonClass(
      ST(base_exception), base_exception_dict, supers);

  // 注入 BaseException.__str__/__repr__，用于让异常对象在 Python 层具备最小的
  // 可读性（str(e) 返回 message）。
  PyDict::Put(base_exception_dict, ST(str),
              PyFunction::NewInstance(&Native_BaseExceptionStr, ST(str)));
  PyDict::Put(base_exception_dict, ST(repr),
              PyFunction::NewInstance(&Native_BaseExceptionRepr, ST(repr)));

  supers = PyList::NewInstance(1);
  supers->SetAndExtendLength(0, base_exception);
  Handle<PyTypeObject> exception =
      Runtime_CreatePythonClass(ST(exception), PyDict::NewInstance(), supers);

  PyDict::Put(builtins_.Get(), ST(base_exception), base_exception);
  PyDict::Put(builtins_.Get(), ST(exception), exception);
}

void BuiltinBootstrapper::RegisterSimpleTypeToBuiltins(
    Handle<PyString> type_name,
    Handle<PyObject> type_base) {
  auto supers = PyList::NewInstance(1);
  supers->SetAndExtendLength(0, type_base);
  auto type_object =
      Runtime_CreatePythonClass(type_name, PyDict::NewInstance(), supers);
  PyDict::Put(builtins_.Get(), type_name, type_object);
}

}  // namespace saauso::internal
