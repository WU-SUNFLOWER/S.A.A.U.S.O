#include <cassert>

#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {
namespace api {

struct HandleScopeImpl {
  explicit HandleScopeImpl(Isolate* isolate)
      : isolate(isolate), isolate_scope(ApiAccess::UnwrapIsolate(isolate)) {}

  Isolate* isolate{nullptr};
  i::Isolate::Scope isolate_scope;
  i::HandleScope handle_scope;
};

struct EscapableHandleScopeImpl {
  explicit EscapableHandleScopeImpl(Isolate* isolate)
      : isolate(isolate), isolate_scope(ApiAccess::UnwrapIsolate(isolate)) {}

  Isolate* isolate{nullptr};
  i::Isolate::Scope isolate_scope;
  i::EscapableHandleScope handle_scope;
};

}  // namespace api

Isolate* Isolate::New(const IsolateCreateParams&) {
  i::Isolate* i_isolate = i::Isolate::New();
  return reinterpret_cast<Isolate*>(i_isolate);
}

void Isolate::Dispose() {
  api::ReleaseCallbackBindingsForIsolate(this);
  auto* i_isolate = ApiAccess::UnwrapIsolate(this);
  i::Isolate::Dispose(i_isolate);
}

void Isolate::ThrowException(Local<Value> exception) {
  if (exception.IsEmpty()) {
    return;
  }

  i::Isolate* i_isolate = ApiAccess::UnwrapIsolate(this);
  i::Isolate::Scope isolate_scope(i_isolate);

  if (i_isolate == nullptr) {
    return;
  }

  i::Handle<i::PyObject> py_exception =
      api::ToInternalObject(i_isolate, exception);
  if (py_exception.is_null()) {
    return;
  }
  i_isolate->exception_state()->Throw(*py_exception);
  api::CapturePendingException(i_isolate);
}

size_t Isolate::ValueRegistrySizeForTesting() const {
  return 0;
}

HandleScope::HandleScope(Isolate* isolate) {
  impl_ = new api::HandleScopeImpl(isolate);
}

HandleScope::~HandleScope() {
  delete reinterpret_cast<api::HandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

EscapableHandleScope::EscapableHandleScope(Isolate* isolate) {
  impl_ = new api::EscapableHandleScopeImpl(isolate);
}

EscapableHandleScope::~EscapableHandleScope() {
  delete reinterpret_cast<api::EscapableHandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

Local<Value> EscapableHandleScope::Escape(Local<Value> value) {
  if (value.IsEmpty()) {
    return value;
  }
  auto* impl = reinterpret_cast<api::EscapableHandleScopeImpl*>(impl_);
  if (impl == nullptr) {
    return Local<Value>();
  }
  i::Handle<i::PyObject> escaped =
      impl->handle_scope.Escape(i::Utils::OpenHandle(value));
  return i::Utils::ToLocal<Value>(escaped);
}

Local<Context> Context::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Context>();
  }
  i::Isolate* i_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (i_isolate == nullptr) {
    return Local<Context>();
  }
  i::Isolate::Scope isolate_scope(i_isolate);
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> escaped = handle_scope.Escape(globals);
  return i::Utils::ToLocal<Context>(escaped);
}

void Context::Enter() {
  auto* i_isolate = i::Isolate::Current();
  auto* entered_contexts = i_isolate->entered_contexts();

  if (i_isolate != nullptr && entered_contexts != nullptr) {
    if (entered_contexts->empty()) {
      i_isolate->Enter();
    }
    entered_contexts->push_back(this);
  }
}

void Context::Exit() {
  auto* i_isolate = i::Isolate::Current();
  auto* entered_contexts = i_isolate->entered_contexts();

  if (i_isolate != nullptr && entered_contexts != nullptr) {
    if (entered_contexts->empty()) {
      assert(false);
      return;
    }
    if (entered_contexts->back() != this) {
      assert(false);
      return;
    }
    entered_contexts->pop_back();
    if (entered_contexts->empty()) {
      i_isolate->Exit();
    }
  }
}

bool Context::Set(Local<String> key, Local<Value> value) {
  if (key.IsEmpty()) {
    return false;
  }

  i::Isolate* i_isolate = i::Isolate::Current();
  i::Isolate::Scope isolate_scope(i_isolate);

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
  i::Isolate::Scope isolate_scope(i_isolate);

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
