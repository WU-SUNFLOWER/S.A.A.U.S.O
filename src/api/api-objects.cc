#include <string>

#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {

Local<Object> Object::New(Isolate* isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> dict =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*dict)));
  return Local<Object>::Cast(internal::Utils::ToLocal<api::RawObject>(escaped));
}

bool Object::Set(Local<String> key, Local<Value> value) {
  i::Isolate* internal_isolate = i::Isolate::Current();

  if (key.IsEmpty()) {
    return false;
  }

  if (internal_isolate == nullptr) {
    return false;
  }

  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyDict(object)) {
    return false;
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
    return false;
  }
  return maybe_put.ToChecked();
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
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
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
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
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
  i::Handle<i::PyObject> self = internal::Utils::OpenHandle(this);
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
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

Local<List> List::New(Isolate* isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyList> list =
      i_isolate->factory()->NewPyList(i::PyList::kMinimumCapacity);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*list)));
  return Local<List>::Cast(internal::Utils::ToLocal<api::RawList>(escaped));
}

int64_t List::Length() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return 0;
  }
  return i::Tagged<i::PyList>::cast(*object)->length();
}

bool List::Push(Local<Value> value) {
  i::Isolate* i_isolate = i::Isolate::Current();
  if (i_isolate == nullptr) {
    return false;
  }

  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return false;
  }
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(i::Tagged<i::PyList>::cast(*object));
  i::PyList::Append(list, api::ToInternalObject(i_isolate, value));
  if (i_isolate->HasPendingException()) {
    api::CapturePendingException(i_isolate);
    return false;
  }
  return true;
}

bool List::Set(int64_t index, Local<Value> value) {
  i::Isolate* i_isolate = i::Isolate::Current();

  if (i_isolate == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return false;
  }
  i::Tagged<i::PyList> list_tagged = i::Tagged<i::PyList>::cast(*object);
  if (index < 0 || index >= list_tagged->length()) {
    return false;
  }
  i::HandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(list_tagged);
  list->Set(index, api::ToInternalObject(i_isolate, value));
  if (i_isolate->HasPendingException()) {
    api::CapturePendingException(i_isolate);
    return false;
  }
  return true;
}

MaybeLocal<Value> List::Get(int64_t index) const {
  i::Isolate* i_isolate = i::Isolate::Current();

  if (i_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyList> list_tagged = i::Tagged<i::PyList>::cast(*object);
  if (index < 0 || index >= list_tagged->length()) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyList> list = i::handle(list_tagged);
  i::Handle<i::PyObject> escaped = handle_scope.Escape(list->Get(index));
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

MaybeLocal<Tuple> Tuple::New(Isolate* isolate, int argc, Local<Value> argv[]) {
  i::Isolate* internal_isolate = i::Isolate::Current();

  if (isolate == nullptr || argc < 0) {
    return MaybeLocal<Tuple>();
  }
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyTuple> tuple = internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    tuple->SetInternal(i, api::ToInternalObject(internal_isolate, arg));
  }
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*tuple)));
  return MaybeLocal<Tuple>(
      Local<Tuple>::Cast(internal::Utils::ToLocal<api::RawTuple>(escaped)));
}

int64_t Tuple::Length() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyTuple(object)) {
    return 0;
  }
  return i::Tagged<i::PyTuple>::cast(*object)->length();
}

MaybeLocal<Value> Tuple::Get(int64_t index) const {
  i::Isolate* internal_isolate = i::Isolate::Current();

  if (internal_isolate == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyTuple(object)) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyTuple> tuple_tagged = i::Tagged<i::PyTuple>::cast(*object);
  if (index < 0 || index >= tuple_tagged->length()) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyTuple> tuple = i::handle(tuple_tagged);
  i::Handle<i::PyObject> escaped = handle_scope.Escape(tuple->Get(index));
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

}  // namespace saauso
