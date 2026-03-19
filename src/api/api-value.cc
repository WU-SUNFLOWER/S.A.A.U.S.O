#include <string>

#include "src/api/api-impl.h"

namespace saauso {

bool Value::IsString() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostString) {
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyString(object);
}

bool Value::IsInteger() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostInteger) {
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPySmi(object);
}

bool Value::IsFloat() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostFloat) {
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyFloat(object);
}

bool Value::IsBoolean() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostBoolean) {
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyTrue(object) || i::IsPyFalse(object);
}

bool Value::ToString(std::string* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostString ||
      impl->kind == api::ValueKind::kScriptSource) {
    *out = impl->string_value;
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return false;
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(object);
  *out = std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
  return true;
}

bool Value::ToInteger(int64_t* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostInteger) {
    *out = impl->int_value;
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return false;
  }
  *out = i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(object));
  return true;
}

bool Value::ToFloat(double* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostFloat) {
    *out = impl->float_value;
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return false;
  }
  *out = i::Tagged<i::PyFloat>::cast(object)->value();
  return true;
}

bool Value::ToBoolean(bool* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostBoolean) {
    *out = impl->bool_value;
    return true;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
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
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return "";
  }
  if (impl->kind == api::ValueKind::kHostString ||
      impl->kind == api::ValueKind::kScriptSource) {
    return impl->string_value;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return "";
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return "";
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(object);
  return std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
}

Local<Integer> Integer::New(Isolate* isolate, int64_t value) {
  return api::WrapHostInteger<Integer>(isolate, value);
}

int64_t Integer::Value() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return 0;
  }
  if (impl->kind == api::ValueKind::kHostInteger) {
    return impl->int_value;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return 0;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return 0;
  }
  return i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(object));
}

Local<Float> Float::New(Isolate* isolate, double value) {
  return api::WrapHostFloat<Float>(isolate, value);
}

double Float::Value() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return 0.0;
  }
  if (impl->kind == api::ValueKind::kHostFloat) {
    return impl->float_value;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return 0.0;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return 0.0;
  }
  return i::Tagged<i::PyFloat>::cast(object)->value();
}

Local<Boolean> Boolean::New(Isolate* isolate, bool value) {
  return api::WrapHostBoolean<Boolean>(isolate, value);
}

bool Boolean::Value() const {
  auto* impl = api::GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == api::ValueKind::kHostBoolean) {
    return impl->bool_value;
  }
  if (impl->kind != api::ValueKind::kVmObject) {
    return false;
  }
  i::Tagged<i::PyObject> object = api::GetObjectTagged(this);
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
  auto* script_impl = api::GetValueImpl(this);
  if (script_impl == nullptr || script_impl->kind != api::ValueKind::kScriptSource) {
    return MaybeLocal<Value>();
  }
  auto* context_impl =
      reinterpret_cast<api::ContextImpl*>(ApiAccess::GetContextImpl(&*context));
  if (context_impl == nullptr || context_impl->globals.is_null()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals = i::handle(context_impl->globals);
  i::MaybeHandle<i::PyObject> maybe_result = i::Runtime_ExecutePythonSourceCode(
      internal_isolate,
      std::string_view(script_impl->string_value.data(),
                       script_impl->string_value.size()),
      globals, globals);
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, result));
}

}  // namespace saauso
