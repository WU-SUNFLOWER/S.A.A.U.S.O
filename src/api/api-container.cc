// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-container.h"
#include "src/api/api-impl.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"

namespace saauso {

MaybeLocal<List> List::New(Isolate* isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyList> list =
      i_isolate->factory()->NewPyList(i::PyList::kMinimumCapacity);
  i::Handle<i::PyObject> escaped = handle_scope.Escape(list);
  return i::Utils::ToLocal<List>(escaped);
}

int64_t List::Length() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return 0;
  }
  return i::Tagged<i::PyList>::cast(*object)->length();
}

Maybe<void> List::Push(Local<Value> value) {
  i::Isolate* i_isolate = i::Isolate::Current();
  assert(i_isolate != nullptr);

  i::HandleScope handle_scope(i_isolate);

  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyObject> i_value = i::Utils::OpenHandle(value);
  if (i_value.is_null()) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyList> list = i::Handle<i::PyList>::cast(object);
  i::PyList::Append(list, i_value, i_isolate);

  return JustVoid();
}

Maybe<void> List::Set(int64_t index, Local<Value> value) {
  i::Isolate* i_isolate = i::Isolate::Current();
  assert(i_isolate != nullptr);

  i::HandleScope handle_scope(i_isolate);

  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyList> list = i::Handle<i::PyList>::cast(object);
  if (index < 0 || index >= list->length()) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyObject> i_value = i::Utils::OpenHandle(value);
  if (i_value.is_null()) {
    return i::kNullMaybe;
  }

  list->Set(index, i_value);
  return JustVoid();
}

MaybeLocal<Value> List::Get(int64_t index) const {
  i::Isolate* i_isolate = i::Isolate::Current();
  assert(i_isolate != nullptr);

  i::EscapableHandleScope handle_scope(i_isolate);

  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyList(object)) {
    return MaybeLocal<Value>();
  }

  i::Tagged<i::PyList> list_tagged = i::Tagged<i::PyList>::cast(*object);
  if (index < 0 || index >= list_tagged->length()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyList> list = i::Handle<i::PyList>::cast(object);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(list->Get(index, i_isolate));
  return i::Utils::ToLocal<Value>(escaped);
}

MaybeLocal<Tuple> Tuple::New(Isolate* isolate, int argc, Local<Value> argv[]) {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate != nullptr);

  i::EscapableHandleScope handle_scope(i_isolate);

  if (argc < 0 || argv == nullptr) {
    return MaybeLocal<Tuple>();
  }

  i::Handle<i::PyTuple> tuple = i_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    i::Handle<i::PyObject> arg = i::Utils::OpenHandle(argv[i]);
    assert(!arg.is_null());
    tuple->SetInternal(i, arg);
  }

  i::Handle<i::PyObject> escaped = handle_scope.Escape(tuple);
  return i::Utils::ToLocal<Tuple>(escaped);
}

int64_t Tuple::Length() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
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
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyTuple(object)) {
    return MaybeLocal<Value>();
  }
  i::Tagged<i::PyTuple> tuple_tagged = i::Tagged<i::PyTuple>::cast(*object);
  if (index < 0 || index >= tuple_tagged->length()) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope(internal_isolate);
  i::Handle<i::PyTuple> tuple = i::Handle<i::PyTuple>::cast(object);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(tuple->Get(index, internal_isolate));
  return i::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
