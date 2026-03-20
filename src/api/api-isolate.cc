#include <cassert>
#include <vector>

#include "src/api/api-impl.h"

namespace saauso {
namespace api {

struct HandleScopeImpl {
  explicit HandleScopeImpl(Isolate* isolate)
      : isolate(isolate),
        isolate_scope(ApiAccess::UnwrapIsolate(isolate)),
        begin_value_size(CurrentValueSize(isolate)) {}

  ~HandleScopeImpl() {
    auto* registry = ApiAccess::ValueRegistry(isolate);
    if (registry == nullptr) {
      return;
    }
    while (registry->size() > begin_value_size) {
      DestroyValue(registry->back());
      registry->pop_back();
    }
  }

  static size_t CurrentValueSize(Isolate* isolate) {
    auto* registry = ApiAccess::ValueRegistry(isolate);
    if (registry == nullptr) {
      return 0;
    }
    return registry->size();
  }

  Isolate* isolate{nullptr};
  i::Isolate::Scope isolate_scope;
  i::HandleScope handle_scope;
  size_t begin_value_size{0};
};

struct EscapableHandleScopeImpl {
  explicit EscapableHandleScopeImpl(Isolate* isolate)
      : isolate(isolate),
        begin_value_size(HandleScopeImpl::CurrentValueSize(isolate)) {}

  ~EscapableHandleScopeImpl() {
    auto* registry = ApiAccess::ValueRegistry(isolate);
    if (registry == nullptr) {
      return;
    }
    std::vector<Value*> escaped_tail;
    while (registry->size() > begin_value_size) {
      Value* value = registry->back();
      bool should_keep = false;
      for (Value* escaped_value : escaped_values) {
        if (escaped_value == value) {
          should_keep = true;
          break;
        }
      }
      if (should_keep) {
        escaped_tail.push_back(value);
        registry->pop_back();
        continue;
      }
      DestroyValue(value);
      registry->pop_back();
    }
    for (auto it = escaped_tail.rbegin(); it != escaped_tail.rend(); ++it) {
      registry->push_back(*it);
    }
  }

  Isolate* isolate{nullptr};
  size_t begin_value_size{0};
  std::vector<Value*> escaped_values;
};

}  // namespace api

Isolate* Isolate::New(const IsolateCreateParams&) {
  i::Isolate* internal_isolate = i::Isolate::New();
  if (internal_isolate == nullptr) {
    return nullptr;
  }
  auto* isolate = new Isolate();
  isolate->internal_isolate_ = internal_isolate;
  isolate->values_ = new std::vector<Value*>();
  isolate->contexts_ = new std::vector<Context*>();
  isolate->entered_contexts_ = new std::vector<Context*>();
  return isolate;
}

void Isolate::Dispose() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(this);
  ApiAccess::DeleteEnteredContextStack(this);
  if (auto* registry = ApiAccess::ValueRegistry(this); registry != nullptr) {
    for (Value* value : *registry) {
      api::DestroyValue(value);
    }
  }
  ApiAccess::DeleteRegisteredValues(this);
  ApiAccess::DeleteRegisteredContexts(this);
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
  i::HandleScope handle_scope;
  i::Handle<i::PyObject> py_exception = api::ToInternalObject(this, exception);
  if (py_exception.is_null()) {
    return;
  }
  internal_isolate->exception_state()->Throw(*py_exception);
  api::CapturePendingException(this);
}

size_t Isolate::ValueRegistrySizeForTesting() const {
  const auto* registry = ApiAccess::ValueRegistry(this);
  if (registry == nullptr) {
    return 0;
  }
  return registry->size();
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
    return value;
  }
  impl->escaped_values.push_back(&*value);
  return value;
}

Local<Context> Context::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Context>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr) {
    return Local<Context>();
  }
  auto* context = new Context(isolate);
  ApiAccess::RegisterValue(isolate, context);
  ApiAccess::RegisterContext(isolate, context);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  ApiAccess::SetContextImpl(context, new api::ContextImpl(globals));
  return ApiAccess::MakeLocal(context);
}

void Context::Enter() {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
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
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
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
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<api::ContextImpl*>(ApiAccess::GetContextImpl(this));
  if (internal_isolate == nullptr || context_impl == nullptr ||
      context_impl->globals.is_null()) {
    return false;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals = i::handle(context_impl->globals);
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
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<api::ContextImpl*>(ApiAccess::GetContextImpl(this));
  if (internal_isolate == nullptr || context_impl == nullptr ||
      context_impl->globals.is_null()) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals = i::handle(context_impl->globals);
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
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, out));
}

Local<Object> Context::Global() {
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<api::ContextImpl*>(ApiAccess::GetContextImpl(this));
  if (internal_isolate == nullptr || context_impl == nullptr ||
      context_impl->globals.is_null()) {
    return Local<Object>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyDict> globals = i::handle(context_impl->globals);
  return Local<Object>::Cast(api::WrapObject<api::RawObject>(
      isolate, i::handle(i::Tagged<i::PyObject>::cast(*globals))));
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
