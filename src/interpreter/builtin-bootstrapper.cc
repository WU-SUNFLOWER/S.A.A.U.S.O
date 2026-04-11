// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/builtin-bootstrapper.h"

#include <string>

#include "src/builtins/builtins-base-exception-methods.h"
#include "src/builtins/builtins-definitions.h"
#include "src/execution/exception-roots.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/templates.h"
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
  EscapableHandleScope scope(isolate_);

  assert(!is_bootstrapped_);
  builtins_ = Global<PyDict>(isolate_, PyDict::New(isolate_));

  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinTypes());
  RETURN_ON_EXCEPTION(isolate_, InstallOddballs());
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinFunctions());
  RETURN_ON_EXCEPTION(isolate_, InstallBuiltinsSelfReference());
  RETURN_ON_EXCEPTION(isolate_, PublishBuiltinExceptionTypes());

  is_bootstrapped_ = true;
  return scope.Escape(builtins_.Get(isolate_));
}

///////////////////////////////////////////////////////////////////////////////////////////

Maybe<void> BuiltinBootstrapper::InstallBuiltinTypes() {
#define BUILTIN_TYPE_ENTRY(name, klass) \
  {name, klass::GetInstance(isolate_)->type_object(isolate_)},

#define BUILTIN_TYPE_LIST(V)   \
  V("object", PyObjectKlass)   \
  V("int", PySmiKlass)         \
  V("str", PyStringKlass)      \
  V("float", PyFloatKlass)     \
  V("list", PyListKlass)       \
  V("bool", PyBooleanKlass)    \
  V("dict", PyDictKlass)       \
  V("tuple", PyTupleKlass)     \
  V("type", PyTypeObjectKlass)

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
    FunctionTemplateInfo func_template(isolate_, entry.func, entry.name);
    Handle<PyFunction> func_object =
        isolate_->factory()->NewPyFunctionWithTemplate(func_template);

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

Maybe<void> BuiltinBootstrapper::PublishBuiltinExceptionTypes() {
#define PUBLISH_EXCEPTION_TYPE(type_enum, name_in_string_table, _) \
  RETURN_ON_EXCEPTION(                                              \
      isolate_, PublishExceptionType(ExceptionType::type_enum,      \
                                     ST(name_in_string_table, isolate_)));

  EXCEPTION_TYPE_LIST(PUBLISH_EXCEPTION_TYPE);

#undef PUBLISH_EXCEPTION_TYPE

  return JustVoid();
}

Maybe<void> BuiltinBootstrapper::PublishExceptionType(
    ExceptionType type,
    Handle<PyString> type_name) {
  Handle<PyTypeObject> type_object = isolate_->exception_roots()->Get(type);
  RETURN_ON_EXCEPTION(isolate_, PyDict::Put(builtins_.Get(isolate_), type_name,
                                            type_object, isolate_));
  return JustVoid();
}

}  // namespace saauso::internal
