// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-context.h"
#include "include/saauso-isolate.h"
#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"

namespace saauso {

Local<Context> Context::New(Isolate* isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);

  i::Handle<i::PyObject> escaped = handle_scope.Escape(globals);
  return i::Utils::ToLocal<Context>(escaped);
}

void Context::Enter() {
  auto* i_isolate = i::Isolate::Current();

  auto& entered_contexts = i_isolate->entered_contexts();

  if (entered_contexts.IsEmpty()) {
    i_isolate->Enter();
  }
  entered_contexts.PushBack(this);
}

void Context::Exit() {
  auto* i_isolate = i::Isolate::Current();
  auto& entered_contexts = i_isolate->entered_contexts();

  if (entered_contexts.IsEmpty()) {
    assert(false);
    return;
  }

  if (entered_contexts.GetBack() != this) {
    assert(false);
    return;
  }

  entered_contexts.PopBack();
  if (entered_contexts.IsEmpty()) {
    i_isolate->Exit();
  }
}

bool Context::Set(Local<String> key, Local<Value> value) {
  if (key.IsEmpty()) {
    return false;
  }

  i::Isolate* i_isolate = i::Isolate::Current();

  i::Handle<i::PyObject> context_object = i::Utils::OpenHandle(this);
  if (i_isolate == nullptr || context_object.is_null() ||
      !i::IsPyDict(context_object)) {
    return false;
  }
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i::handle(i::Tagged<i::PyDict>::cast(*context_object));
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> py_value = api::ToInternalObject(i_isolate, value);
  auto maybe_set = i::PyDict::Put(
      globals, i::handle(i::Tagged<i::PyObject>::cast(*py_key)), py_value);
  if (maybe_set.IsNothing()) {
    api::CapturePendingException(i_isolate);
    return false;
  }
  return maybe_set.ToChecked();
}

MaybeLocal<Value> Context::Get(Local<String> key) {
  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Isolate* i_isolate = i::Isolate::Current();

  i::Handle<i::PyObject> context_object = i::Utils::OpenHandle(this);
  if (i_isolate == nullptr || context_object.is_null() ||
      !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i::handle(i::Tagged<i::PyDict>::cast(*context_object));
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> out;
  auto maybe_found =
      globals->Get(i::handle(i::Tagged<i::PyObject>::cast(*py_key)), out);
  if (maybe_found.IsNothing()) {
    api::CapturePendingException(i_isolate);
    return MaybeLocal<Value>();
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(out);
  return MaybeLocal<Value>(i::Utils::ToLocal<Value>(escaped));
}

Local<Object> Context::Global() {
  i::Handle<i::PyObject> context_object = i::Utils::OpenHandle(this);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return Local<Object>();
  }
  return Local<Object>::Cast(i::Utils::ToLocal<api::RawObject>(context_object));
}

ContextScope::ContextScope(Local<Context> context) : context_(context) {
  if (!context_.IsEmpty()) {
    context_->Enter();
  }
}

ContextScope::~ContextScope() {
  if (!context_.IsEmpty()) {
    context_->Exit();
  }
}

}  // namespace saauso
