#include <cassert>
#include <vector>

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
  i::Isolate* internal_isolate = i::Isolate::New();
  if (internal_isolate == nullptr) {
    return nullptr;
  }
  auto* isolate = new Isolate();
  isolate->internal_isolate_ = internal_isolate;
  isolate->entered_contexts_ = new std::vector<Context*>();
  api::RegisterPublicIsolate(isolate);
  return isolate;
}

void Isolate::Dispose() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(this);
  ApiAccess::DeleteEnteredContextStack(this);
  api::UnregisterPublicIsolate(this);
  api::ReleaseCallbackBindingsForIsolate(this);
  i::Isolate::Dispose(internal_isolate);
  internal_isolate_ = nullptr;
  delete this;
}

void Isolate::ThrowException(Local<Value> exception) {
  if (exception.IsEmpty()) {
    return;
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(this);
  if (internal_isolate == nullptr) {
    return;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::Handle<i::PyObject> py_exception = api::ToInternalObject(this, exception);
  if (py_exception.is_null()) {
    return;
  }
  internal_isolate->exception_state()->Throw(*py_exception);
  api::CapturePendingException(this);
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
      impl->handle_scope.Escape(internal::Utils::OpenHandle(value));
  return internal::Utils::ToLocal<Value>(escaped);
}

Local<Context> Context::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Context>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return Local<Context>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*globals)));
  return internal::Utils::ToLocal<Context>(escaped);
}

void Context::Enter() {
  Isolate* isolate = api::CurrentPublicIsolate();
  auto* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* entered_contexts = ApiAccess::EnteredContextStack(isolate);
  if (internal_isolate != nullptr && entered_contexts != nullptr) {
    if (entered_contexts->empty()) {
      internal_isolate->Enter();
    }
    entered_contexts->push_back(this);
  }
}

void Context::Exit() {
  Isolate* isolate = api::CurrentPublicIsolate();
  auto* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* entered_contexts = ApiAccess::EnteredContextStack(isolate);
  if (internal_isolate != nullptr && entered_contexts != nullptr) {
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
      internal_isolate->Exit();
    }
  }
}

bool Context::Set(Local<String> key, Local<Value> value) {
  if (key.IsEmpty()) {
    return false;
  }
  Isolate* isolate = api::CurrentPublicIsolate();
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(this);
  if (internal_isolate == nullptr || context_object.is_null() ||
      !i::IsPyDict(context_object)) {
    return false;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i::handle(i::Tagged<i::PyDict>::cast(*context_object));
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> py_value = api::ToInternalObject(isolate, value);
  auto maybe_set = i::PyDict::Put(
      globals, i::handle(i::Tagged<i::PyObject>::cast(*py_key)), py_value);
  if (maybe_set.IsNothing()) {
    api::CapturePendingException(isolate);
    return false;
  }
  return maybe_set.ToChecked();
}

MaybeLocal<Value> Context::Get(Local<String> key) {
  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = api::CurrentPublicIsolate();
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(this);
  if (internal_isolate == nullptr || context_object.is_null() ||
      !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      i::handle(i::Tagged<i::PyDict>::cast(*context_object));
  i::Handle<i::PyString> py_key = i::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  i::Handle<i::PyObject> out;
  auto maybe_found =
      globals->Get(i::handle(i::Tagged<i::PyObject>::cast(*py_key)), out);
  if (maybe_found.IsNothing()) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(out);
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

Local<Object> Context::Global() {
  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(this);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return Local<Object>();
  }
  return Local<Object>::Cast(
      internal::Utils::ToLocal<api::RawObject>(context_object));
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
