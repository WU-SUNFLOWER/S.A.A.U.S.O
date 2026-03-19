#include <string>

#include "src/api/api-impl.h"

namespace saauso {

Local<Object> Object::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Object>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> dict =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  return Local<Object>::Cast(api::WrapObject<api::RawObject>(
      isolate, i::handle(i::Tagged<i::PyObject>::cast(*dict))));
}

bool Object::Set(Local<String> key, Local<Value> value) {
  if (key.IsEmpty()) {
    return false;
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return false;
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyDict(object_tagged)) {
    return false;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> dict = i::handle(i::Tagged<i::PyDict>::cast(object_tagged));
  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> py_value = api::ToInternalObject(isolate, value);
  auto maybe_put = i::PyDict::Put(
      dict, i::handle(i::Tagged<i::PyObject>::cast(*py_key)), py_value);
  if (maybe_put.IsNothing()) {
    api::CapturePendingException(isolate);
    return false;
  }
  return maybe_put.ToChecked();
}

MaybeLocal<Value> Object::Get(Local<String> key) {
  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyDict(object_tagged)) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> dict = i::handle(i::Tagged<i::PyDict>::cast(object_tagged));
  std::string key_value = key->Value();
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key_value.data(), static_cast<int64_t>(key_value.size()));
  i::Handle<i::PyObject> out;
  auto maybe_found =
      dict->Get(i::handle(i::Tagged<i::PyObject>::cast(*py_key)), out);
  if (maybe_found.IsNothing()) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, out));
}

MaybeLocal<Value> Object::CallMethod(Local<Context> context,
                                     Local<String> name,
                                     int argc,
                                     Local<Value> argv[]) {
  if (context.IsEmpty() || name.IsEmpty() || argc < 0) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyObject> self_tagged = api::GetObjectTagged(this);
  if (self_tagged.is_null()) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyObject> self = i::handle(self_tagged);
  std::string name_value = name->Value();
  i::Handle<i::PyString> py_name = i::PyString::NewInstance(
      name_value.data(), static_cast<int64_t>(name_value.size()));
  i::Handle<i::PyObject> self_or_null;
  i::MaybeHandle<i::PyObject> maybe_callee = i::PyObject::GetAttrForCall(
      self, i::handle(i::Tagged<i::PyObject>::cast(*py_name)), self_or_null);
  i::Handle<i::PyObject> callee;
  if (!maybe_callee.ToHandle(&callee)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyTuple> py_args = internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    py_args->SetInternal(i, api::ToInternalObject(isolate, arg));
  }
  i::Handle<i::PyDict> py_kwargs =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);

  i::MaybeHandle<i::PyObject> maybe_result =
      i::PyObject::Call(internal_isolate, callee, self_or_null,
                        i::handle(i::Tagged<i::PyObject>::cast(*py_args)),
                        i::handle(i::Tagged<i::PyObject>::cast(*py_kwargs)));

  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, result));
}

Local<List> List::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<List>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list =
      internal_isolate->factory()->NewPyList(i::PyList::kMinimumCapacity);
  return Local<List>::Cast(api::WrapObject<api::RawList>(
      isolate, i::handle(i::Tagged<i::PyObject>::cast(*list))));
}

int64_t List::Length() const {
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyList(object_tagged)) {
    return 0;
  }
  return i::Tagged<i::PyList>::cast(object_tagged)->length();
}

bool List::Push(Local<Value> value) {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return false;
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyList(object_tagged)) {
    return false;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(i::Tagged<i::PyList>::cast(object_tagged));
  i::PyList::Append(list, api::ToInternalObject(isolate, value));
  if (internal_isolate->HasPendingException()) {
    api::CapturePendingException(isolate);
    return false;
  }
  return true;
}

bool List::Set(int64_t index, Local<Value> value) {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return false;
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyList(object_tagged)) {
    return false;
  }
  i::Tagged<i::PyList> list_tagged = i::Tagged<i::PyList>::cast(object_tagged);
  if (index < 0 || index >= list_tagged->length()) {
    return false;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(list_tagged);
  list->Set(index, api::ToInternalObject(isolate, value));
  if (internal_isolate->HasPendingException()) {
    api::CapturePendingException(isolate);
    return false;
  }
  return true;
}

MaybeLocal<Value> List::Get(int64_t index) const {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyList(object_tagged)) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyList> list_tagged = i::Tagged<i::PyList>::cast(object_tagged);
  if (index < 0 || index >= list_tagged->length()) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(list_tagged);
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, list->Get(index)));
}

MaybeLocal<Tuple> Tuple::New(Isolate* isolate, int argc, Local<Value> argv[]) {
  if (isolate == nullptr || argc < 0) {
    return MaybeLocal<Tuple>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyTuple> tuple = internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    tuple->SetInternal(i, api::ToInternalObject(isolate, arg));
  }
  return MaybeLocal<Tuple>(Local<Tuple>::Cast(api::WrapObject<api::RawTuple>(
      isolate, i::handle(i::Tagged<i::PyObject>::cast(*tuple)))));
}

int64_t Tuple::Length() const {
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyTuple(object_tagged)) {
    return 0;
  }
  return i::Tagged<i::PyTuple>::cast(object_tagged)->length();
}

MaybeLocal<Value> Tuple::Get(int64_t index) const {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyObject> object_tagged = api::GetObjectTagged(this);
  if (object_tagged.is_null() || !i::IsPyTuple(object_tagged)) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyTuple> tuple_tagged = i::Tagged<i::PyTuple>::cast(object_tagged);
  if (index < 0 || index >= tuple_tagged->length()) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyTuple> tuple = i::handle(tuple_tagged);
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, tuple->Get(index)));
}

}  // namespace saauso
