// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cassert>

#include "include/saauso-context.h"
#include "include/saauso-isolate.h"
#include "include/saauso-primitive.h"
#include "src/api/api-exception-support.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"

namespace saauso {

///////////////////////////////////////////////////////////////////////
// Context::Scope 实现

Context::Scope::Scope(Local<Context> context) : context_(context) {
  assert(!context_.IsEmpty());
  context_->Enter();
}

Context::Scope::~Scope() {
  assert(!context_.IsEmpty());
  context_->Exit();
}

///////////////////////////////////////////////////////////////////////
// Context 本体实现

Local<Context> Context::New(Isolate* isolate) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyDict> globals =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);

  i::Handle<i::PyObject> escaped = handle_scope.Escape(globals);
  return api::Utils::ToLocal<Context>(escaped);
}

void Context::Enter() {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();
  auto& entered_contexts = i_isolate->entered_contexts();
  entered_contexts.PushBack(this);
}

void Context::Exit() {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();
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
}

Maybe<void> Context::Set(Local<String> key, Local<Value> value) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::HandleScope handle_scope(i_isolate);

  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(this);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyDict> globals = i::Handle<i::PyDict>::cast(context_object);

  if (key.IsEmpty() || value.IsEmpty()) {
    return i::kNullMaybe;
  }

  i::Handle<i::PyString> i_key =
      i::PyString::New(i_isolate, key->Value().data(),
                       static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> i_value = api::Utils::OpenHandle(value);

  auto maybe_set = i::PyDict::Put(globals, i_key, i_value, i_isolate);
  if (maybe_set.IsNothing()) {
    return api::FinalizePendingExceptionAtApiBoundaryAndReturnNothing(
        i_isolate);
  }

  return JustVoid();
}

MaybeLocal<Value> Context::Get(Local<String> key) {
  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(this);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }
  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyDict> globals = i::Handle<i::PyDict>::cast(context_object);
  i::Handle<i::PyString> py_key =
      i::PyString::New(i_isolate, key->Value().data(),
                       static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> out;
  auto maybe_found = i::PyDict::Get(globals, py_key, out, i_isolate);
  if (maybe_found.IsNothing()) {
    return api::FinalizePendingExceptionAtApiBoundaryAndReturnEmptyLocal<Value>(
        i_isolate);
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(out);
  return api::Utils::ToLocal<Value>(escaped);
}

Local<Object> Context::Global() {
  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(this);
  return api::Utils::ToLocal<Object>(context_object);
}

}  // namespace saauso
