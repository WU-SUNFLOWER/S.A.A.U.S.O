#include <string>

#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {

MaybeLocal<Object> Object::New(Isolate* isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> dict =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*dict)));
  return MaybeLocal<Object>(
      Local<Object>::Cast(i::Utils::ToLocal<api::RawObject>(escaped)));
}

Maybe<void> Object::Set(Local<String> key, Local<Value> value) {
  i::Isolate* internal_isolate = i::Isolate::Current();

  if (key.IsEmpty()) {
    return i::kNullMaybe;
  }

  if (internal_isolate == nullptr) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyDict(object)) {
    return i::kNullMaybe;
  }

  i::HandleScope handle_scope;
  i::Handle<i::PyDict> dict = i::handle(i::Tagged<i::PyDict>::cast(*object));
  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> py_value =
      api::ToInternalObject(internal_isolate, value);
  auto maybe_put = i::PyDict::Put(
      dict, i::handle(i::Tagged<i::PyObject>::cast(*py_key)), py_value);
  if (maybe_put.IsNothing()) {
    api::CapturePendingException(internal_isolate);
    return i::kNullMaybe;
  }
  if (!maybe_put.ToChecked()) {
    return i::kNullMaybe;
  }
  return JustVoid();
}

MaybeLocal<Value> Object::Get(Local<String> key) {
  i::Isolate* internal_isolate = i::Isolate::Current();

  i::EscapableHandleScope handle_scope;

  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyDict(object)) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyDict> dict = i::handle(i::Tagged<i::PyDict>::cast(*object));
  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> out;
  auto maybe_found =
      dict->Get(i::handle(i::Tagged<i::PyObject>::cast(*py_key)), out);
  if (maybe_found.IsNothing()) {
    api::CapturePendingException(internal_isolate);
    return MaybeLocal<Value>();
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(out);
  return MaybeLocal<Value>(i::Utils::ToLocal<Value>(escaped));
}

MaybeLocal<Value> Object::CallMethod(Local<Context> context,
                                     Local<String> name,
                                     int argc,
                                     Local<Value> argv[]) {
  i::Isolate* internal_isolate = i::Isolate::Current();

  if (context.IsEmpty() || name.IsEmpty() || argc < 0) {
    return MaybeLocal<Value>();
  }
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> self = i::Utils::OpenHandle(this);
  if (self.is_null()) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope;
  std::string name_value = name->Value();
  i::Handle<i::PyString> py_name = i::PyString::NewInstance(
      name_value.data(), static_cast<int64_t>(name_value.size()));
  i::Handle<i::PyObject> self_or_null;
  i::MaybeHandle<i::PyObject> maybe_callee = i::PyObject::GetAttrForCall(
      self, i::handle(i::Tagged<i::PyObject>::cast(*py_name)), self_or_null);
  i::Handle<i::PyObject> callee;
  if (!maybe_callee.ToHandle(&callee)) {
    api::CapturePendingException(internal_isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyTuple> py_args = internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    py_args->SetInternal(i, api::ToInternalObject(internal_isolate, arg));
  }
  i::Handle<i::PyDict> py_kwargs =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);

  i::MaybeHandle<i::PyObject> maybe_result =
      i::PyObject::Call(internal_isolate, callee, self_or_null,
                        i::handle(i::Tagged<i::PyObject>::cast(*py_args)),
                        i::handle(i::Tagged<i::PyObject>::cast(*py_kwargs)));

  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(internal_isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return MaybeLocal<Value>(i::Utils::ToLocal<Value>(escaped));
}

}  // namespace saauso
