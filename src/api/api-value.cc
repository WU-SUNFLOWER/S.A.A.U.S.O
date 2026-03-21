// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "include/saauso-primitive.h"
#include "include/saauso-script.h"
#include "src/api/api-impl.h"

namespace saauso {

bool Value::IsString() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyString(object);
}

bool Value::IsInteger() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPySmi(object);
}

bool Value::IsFloat() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyFloat(object);
}

bool Value::IsBoolean() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyTrue(object) || i::IsPyFalse(object);
}

bool Value::ToString(std::string* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return false;
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  *out = std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
  return true;
}

bool Value::ToInteger(int64_t* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return false;
  }
  *out = i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object));
  return true;
}

bool Value::ToFloat(double* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return false;
  }
  *out = i::Tagged<i::PyFloat>::cast(*object)->value();
  return true;
}

bool Value::ToBoolean(bool* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  if (i::IsPyTrue(object)) {
    *out = true;
    return true;
  }
  if (i::IsPyFalse(object)) {
    *out = false;
    return true;
  }
  return false;
}

Local<String> String::New(Isolate* isolate, std::string_view value) {
  return api::WrapHostString<String>(isolate, std::string(value));
}

std::string String::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return "";
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  return std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
}

Local<Integer> Integer::New(Isolate* isolate, int64_t value) {
  return api::WrapHostInteger<Integer>(isolate, value);
}

int64_t Integer::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return 0;
  }
  return i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object));
}

Local<Float> Float::New(Isolate* isolate, double value) {
  return api::WrapHostFloat<Float>(isolate, value);
}

double Float::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return 0.0;
  }
  return i::Tagged<i::PyFloat>::cast(*object)->value();
}

Local<Boolean> Boolean::New(Isolate* isolate, bool value) {
  return api::WrapHostBoolean<Boolean>(isolate, value);
}

bool Boolean::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyTrue(object);
}

MaybeLocal<Script> Script::Compile(Isolate* isolate, Local<String> source) {
  if (isolate == nullptr || source.IsEmpty()) {
    return MaybeLocal<Script>();
  }
  if (!Local<Value>::Cast(source)->IsString()) {
    return MaybeLocal<Script>();
  }
#if SAAUSO_ENABLE_CPYTHON_COMPILER
  return MaybeLocal<Script>(api::WrapScriptSource(isolate, source->Value()));
#else
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Runtime_ThrowError(
      i::ExceptionType::kRuntimeError,
      "Script::Compile requires CPython frontend compiler support");
  api::CapturePendingException(isolate);
  return MaybeLocal<Script>();
#endif
}

MaybeLocal<Value> Script::Run(Local<Context> context) {
  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> script_object = internal::Utils::OpenHandle(this);
  if (script_object.is_null() || !i::IsPyString(script_object)) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(context);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = api::CurrentPublicIsolate();
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyString> source_string =
      i::Tagged<i::PyString>::cast(*script_object);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i::handle(i::Tagged<i::PyDict>::cast(*context_object));
  i::MaybeHandle<i::PyObject> maybe_result = i::Runtime_ExecutePythonSourceCode(
      internal_isolate,
      std::string_view(source_string->buffer(),
                       static_cast<size_t>(source_string->length())),
      globals, globals);
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

}  // namespace saauso
