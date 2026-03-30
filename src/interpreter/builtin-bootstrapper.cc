// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/builtin-bootstrapper.h"

#include <string>

#include "src/builtins/builtins-base-exception-methods.h"
#include "src/builtins/builtins-definitions.h"
#include "src/execution/exception-types.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-base-exception-klass.h"
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
#include "src/objects/templates.h"
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
  Handle<PyString> name;
  Tagged<PyObject> value;
};

struct BuiltinFunctionEntry {
  Handle<PyString> name;
  NativeFuncPointer func;
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////////////////

BuiltinBootstrapper::BuiltinBootstrapper(Isolate* isolate)
    : isolate_(isolate) {}

MaybeHandle<PyDict> BuiltinBootstrapper::CreateBuiltins() {
  EscapableHandleScope scope;

  assert(!is_bootstrapped_);
  builtins_ = Global<PyDict>(isolate_, PyDict::New(isolate_));

  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinTypes());
  RETURN_ON_EXCEPTION(isolate_, InstallOddballs());
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinFunctions());
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinsSelfReference());
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinExceptionTypes());

  is_bootstrapped_ = true;
  return scope.Escape(builtins_.Get(isolate_));
}

///////////////////////////////////////////////////////////////////////////////////////////

Maybe<void> BuiltinBootstrapper::InstallBuiltinTypes() {
#define BUILTIN_TYPE_ENTRY(name, klass) \
  {name, klass::GetInstance(isolate_)->type_object()},

#define BUILTIN_TYPE_LIST(V)   \
  V("object", PyObjectKlass)   \
  V("int", PySmiKlass)         \
  V("str", PyStringKlass)      \
  V("float", PyFloatKlass)     \
  V("list", PyListKlass)       \
  V("bool", PyBooleanKlass)    \
  V("dict", PyDictKlass)       \
  V("tuple", PyTupleKlass)     \
  V("type", PyTypeObjectKlass) \
  V("BaseException", PyBaseExceptionKlass)

  const BuiltinTypeEntry entries[] = {BUILTIN_TYPE_LIST(BUILTIN_TYPE_ENTRY)};

#undef BUILTIN_TYPE_LIST
#undef BUILTIN_TYPE_ENTRY

  auto builtins_handle = builtins_.Get(isolate_);
  for (const auto& entry : entries) {
    RETURN_ON_EXCEPTION(
        isolate_,
        PyDict::Put(builtins_handle, PyString::New(isolate_, entry.name),
                    entry.type_object, isolate_));
  }

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::InstallOddballs() {
  // 注册解释器侧可见的单例对象。
  const BuiltinOddballEntry entries[] = {
      {ST(true_symbol, isolate_), isolate_->py_true_object()},
      {ST(false_symbol, isolate_), isolate_->py_false_object()},
      {ST(none_symbol, isolate_), isolate_->py_none_object()},
  };

  auto builtins_handle = builtins_.Get(isolate_);
  for (const auto& entry : entries) {
    RETURN_ON_EXCEPTION(isolate_,
                        PyDict::Put(builtins_handle, entry.name,
                                    handle(entry.value, isolate_), isolate_));
  }

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::InstallBuiltinFunctions() {
#define BUILTIN_FUNC_ENTRY(func_name_in_string_table, func_cpp_symbol) \
  {ST(func_name_in_string_table, isolate_),                            \
   &BUILTIN_FUNC_NAME(func_cpp_symbol)},

  const BuiltinFunctionEntry entries[] = {
      BUILTIN_FUNC_LIST(BUILTIN_FUNC_ENTRY)};

  auto builtins_handle = builtins_.Get(isolate_);
  for (const auto& entry : entries) {
    Handle<PyFunction> func_object;
    FunctionTemplateInfo func_template(isolate_, entry.func, entry.name);
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate_, func_object,
        isolate_->factory()->NewPyFunctionWithTemplate(func_template));

    RETURN_ON_EXCEPTION(isolate_, PyDict::Put(builtins_handle, entry.name,
                                              func_object, isolate_));
  }

#undef BUILTIN_FUNC_LIST
#undef BUILTIN_FUNC_ENTRY

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::InstallBuiltinsSelfReference() {
  auto builtins_handle = builtins_.Get(isolate_);
  RETURN_ON_EXCEPTION(isolate_,
                      PyDict::Put(builtins_handle, ST(builtins, isolate_),
                                  builtins_handle, isolate_));
  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::InstallBuiltinExceptionTypes() {
  // 先创建 Exception（派生自内建类型 BaseException），
  // 确保后续代码中 Exception 可用。
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinBasicExceptionTypes());

  auto builtins_handle = builtins_.Get(isolate_);
  Tagged<PyObject> exception_type_tagged;

  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, found,
      builtins_handle->GetTagged(ST(exception, isolate_), exception_type_tagged,
                                 isolate_));

  if (!found) {
    Runtime_ThrowError(isolate_, ExceptionType::kRuntimeError,
                       "can't find exception type");
    return kNullMaybe;
  }

  Handle<PyObject> exception_type = handle(exception_type_tagged, isolate_);

#define REGISTER_NORMAL_EXCEPTION_TYPE(ignore1, name_in_string_table, ignore2) \
  RETURN_ON_EXCEPTION(                                                         \
      isolate_, RegisterSimpleTypeToBuiltins(                                  \
                    ST(name_in_string_table, isolate_), exception_type));

  NORMAL_EXCEPTION_TYPE(REGISTER_NORMAL_EXCEPTION_TYPE);

#undef REGISTER_NORMAL_EXCEPTION_TYPE

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::InstallBuiltinBasicExceptionTypes() {
  Handle<PyTypeObject> base_exception =
      PyBaseExceptionKlass::GetInstance(isolate_)->type_object();

  auto supers = PyList::New(isolate_, 1);
  supers->SetAndExtendLength(0, base_exception);

  Handle<PyTypeObject> exception;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, exception,
      Runtime_CreatePythonClass(isolate_, ST(exception, isolate_),
                                PyDict::New(isolate_), supers));

  // 注入 Exception 类型到 builtin 字典
  RETURN_ON_EXCEPTION(
      isolate_, PyDict::Put(builtins_.Get(isolate_), ST(exception, isolate_),
                            exception, isolate_));

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::RegisterSimpleTypeToBuiltins(
    Handle<PyString> type_name,
    Handle<PyObject> type_base) {
  auto supers = PyList::New(isolate_, 1);
  supers->SetAndExtendLength(0, type_base);

  Handle<PyObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, type_object,
      Runtime_CreatePythonClass(isolate_, type_name, PyDict::New(isolate_),
                                supers));

  RETURN_ON_EXCEPTION(isolate_, PyDict::Put(builtins_.Get(isolate_), type_name,
                                            type_object, isolate_));

  return JustVoid();
}

}  // namespace saauso::internal
