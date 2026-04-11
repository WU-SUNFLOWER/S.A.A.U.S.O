#include <string>

#include "include/saauso-object.h"
#include "include/saauso-primitive.h"
#include "src/api/api-exception-support.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso {

MaybeLocal<Object> Object::New(Isolate* isolate) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyDict> dict =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> escaped = handle_scope.Escape(dict);
  return api::Utils::ToLocal<Object>(escaped);
}

Maybe<void> Object::Set(Local<String> key, Local<Value> value) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::HandleScope handle_scope(i_isolate);

  if (key.IsEmpty() || value.IsEmpty()) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyDict(object)) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyDict> dict = i::Handle<i::PyDict>::cast(object);

  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::New(
      i_isolate, key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> py_value = api::Utils::OpenHandle(value);

  auto maybe_put = i::PyDict::Put(dict, py_key, py_value, i_isolate);
  if (maybe_put.IsNothing()) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return i::kNullMaybe;
  }

  return JustVoid();
}

MaybeLocal<Value> Object::Get(Local<String> key) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::EscapableHandleScope handle_scope(i_isolate);

  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyDict(object)) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyDict> dict = i::Handle<i::PyDict>::cast(object);
  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::New(
      i_isolate, key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> out;
  auto maybe_found = i::PyDict::Get(dict, py_key, out, i_isolate);
  if (maybe_found.IsNothing()) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return {};
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(out);
  return api::Utils::ToLocal<Value>(escaped);
}

MaybeLocal<Value> Object::CallMethod(Local<Context> context,
                                     Local<String> name,
                                     int argc,
                                     Local<Value> argv[]) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::EscapableHandleScope handle_scope(i_isolate);

  if (context.IsEmpty() || name.IsEmpty() || argc < 0) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> self = api::Utils::OpenHandle(this);
  if (self.is_null()) {
    return MaybeLocal<Value>();
  }

  std::string name_value = name->Value();
  i::Handle<i::PyString> py_name = i::PyString::New(
      i_isolate, name_value.data(), static_cast<int64_t>(name_value.size()));

  i::Handle<i::PyObject> self_or_null;
  i::Handle<i::PyObject> callee;

  i::MaybeHandle<i::PyObject> maybe_callee =
      i::PyObject::GetAttrForCall(i_isolate, self, py_name, self_or_null);
  if (!maybe_callee.ToHandle(&callee)) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return {};
  }

  i::Handle<i::PyTuple> py_args;
  if (argc > 0) {
    py_args = i_isolate->factory()->NewPyTuple(argc);
    for (int i = 0; i < argc; ++i) {
      assert(!argv[i].IsEmpty());
      py_args->SetInternal(i, api::Utils::OpenHandle(argv[i]));
    }
  }

  i::MaybeHandle<i::PyObject> maybe_result = i::PyObject::Call(
      i_isolate, callee, self_or_null, py_args, i::Handle<i::PyObject>::null());

  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return {};
  }

  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return api::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
