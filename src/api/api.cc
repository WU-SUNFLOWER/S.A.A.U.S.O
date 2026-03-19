// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "include/saauso-embedder.h"
#include "src/api/api-inl.h"
#include "src/execution/exception-types.h"
#include "src/handles/handle-scopes.h"
#include "src/heap/factory.h"
#include "src/objects/object-checkers.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/templates.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-exec.h"
#include "src/runtime/runtime-py-function.h"

namespace saauso {

namespace {

class RawValue final : public Value {};

enum class ValueKind {
  kNone,
  kHostString,
  kHostInteger,
  kScriptSource,
  kVmObject,
};

// Embedder Value 的私有实现体。
// 该结构用于在不暴露内部对象布局的前提下保存“值来源”和可选缓存数据。
struct ValueImpl {
  ValueKind kind{ValueKind::kNone};
  std::string string_value;
  int64_t int_value{0};
  internal::Tagged<internal::PyObject> object{
      internal::Tagged<internal::PyObject>::null()};
};

// Context 的最小私有实现：当前只保存 globals 字典。
// 后续若接入多上下文栈或安全策略，可在此结构体中扩展。
struct ContextImpl {
  explicit ContextImpl(internal::Handle<internal::PyDict> globals)
      : globals(globals.is_null() ? internal::Tagged<internal::PyDict>::null()
                                  : *globals) {}

  internal::Tagged<internal::PyDict> globals;
};

struct FunctionCallbackInfoImpl {
  Isolate* isolate{nullptr};
  internal::Handle<internal::PyObject> receiver;
  internal::Handle<internal::PyTuple> args;
  Local<Value> return_value;
};

struct CallbackSlot {
  FunctionCallback callback{nullptr};
  Isolate* isolate{nullptr};
};

constexpr int kMaxEmbedderCallbackSlots = 16;
CallbackSlot g_embedder_callback_slots[kMaxEmbedderCallbackSlots];

int AllocateCallbackSlot(Isolate* isolate, FunctionCallback callback) {
  for (int i = 0; i < kMaxEmbedderCallbackSlots; ++i) {
    if (g_embedder_callback_slots[i].callback == nullptr) {
      g_embedder_callback_slots[i].callback = callback;
      g_embedder_callback_slots[i].isolate = isolate;
      return i;
    }
  }
  return -1;
}

void ReleaseCallbackSlotsForIsolate(Isolate* isolate) {
  for (int i = 0; i < kMaxEmbedderCallbackSlots; ++i) {
    if (g_embedder_callback_slots[i].isolate == isolate) {
      g_embedder_callback_slots[i].callback = nullptr;
      g_embedder_callback_slots[i].isolate = nullptr;
    }
  }
}

void DestroyValue(Value* value) {
  if (value == nullptr) {
    return;
  }
  delete reinterpret_cast<ValueImpl*>(ApiAccess::GetValueImpl(value));
  ApiAccess::SetValueImpl(value, nullptr);
  ApiAccess::DeleteValue(value);
}

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
  internal::Isolate::Scope isolate_scope;
  internal::HandleScope handle_scope;
  size_t begin_value_size{0};
};

template <typename T>
Local<T> WrapObject(Isolate* isolate,
                    internal::Handle<internal::PyObject> object) {
  if (isolate == nullptr || object.is_null()) {
    return Local<T>();
  }
  auto* value = new T();
  ApiAccess::RegisterValue(isolate, value);
  ApiAccess::SetValueIsolate(value, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kVmObject;
  impl->object = *object;
  ApiAccess::SetValueImpl(value, impl);
  return ApiAccess::MakeLocal(value);
}

template <typename T>
Local<T> WrapHostString(Isolate* isolate, std::string value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  auto* obj = new T();
  ApiAccess::RegisterValue(isolate, obj);
  ApiAccess::SetValueIsolate(obj, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kHostString;
  impl->string_value = std::move(value);
  ApiAccess::SetValueImpl(obj, impl);
  return ApiAccess::MakeLocal(obj);
}

template <typename T>
Local<T> WrapHostInteger(Isolate* isolate, int64_t value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  auto* obj = new T();
  ApiAccess::RegisterValue(isolate, obj);
  ApiAccess::SetValueIsolate(obj, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kHostInteger;
  impl->int_value = value;
  ApiAccess::SetValueImpl(obj, impl);
  return ApiAccess::MakeLocal(obj);
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
Local<Script> WrapScriptSource(Isolate* isolate, std::string source) {
  if (isolate == nullptr) {
    return Local<Script>();
  }
  auto* script = new Script();
  ApiAccess::RegisterValue(isolate, script);
  ApiAccess::SetValueIsolate(script, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kScriptSource;
  impl->string_value = std::move(source);
  ApiAccess::SetValueImpl(script, impl);
  return ApiAccess::MakeLocal(script);
}
#endif

ValueImpl* GetValueImpl(const Value* value) {
  return reinterpret_cast<ValueImpl*>(ApiAccess::GetValueImpl(value));
}

internal::Tagged<internal::PyObject> GetObjectTagged(const Value* value) {
  auto* impl = GetValueImpl(value);
  if (impl == nullptr || impl->kind != ValueKind::kVmObject) {
    return internal::Tagged<internal::PyObject>::null();
  }
  return impl->object;
}

internal::Handle<internal::PyObject> ToInternalObject(Isolate* isolate,
                                                      Local<Value> value) {
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (value.IsEmpty()) {
    return internal::handle(internal::Tagged<internal::PyObject>::cast(
        internal_isolate->py_none_object()));
  }
  ValueImpl* impl = GetValueImpl(&*value);
  if (impl == nullptr) {
    return internal::handle(internal::Tagged<internal::PyObject>::cast(
        internal_isolate->py_none_object()));
  }
  if (impl->kind == ValueKind::kHostString ||
      impl->kind == ValueKind::kScriptSource) {
    internal::Handle<internal::PyString> py_string =
        internal::PyString::NewInstance(
            impl->string_value.data(),
            static_cast<int64_t>(impl->string_value.size()));
    return internal::handle(
        internal::Tagged<internal::PyObject>::cast(*py_string));
  }
  if (impl->kind == ValueKind::kHostInteger) {
    return internal::handle(internal::Tagged<internal::PyObject>::cast(
        internal::PySmi::FromInt(impl->int_value)));
  }
  if (impl->kind == ValueKind::kVmObject && !impl->object.is_null()) {
    return internal::handle(impl->object);
  }
  return internal::handle(internal::Tagged<internal::PyObject>::cast(
      internal_isolate->py_none_object()));
}

Local<Value> WrapRuntimeResult(Isolate* isolate,
                               internal::Handle<internal::PyObject> result) {
  if (result.is_null()) {
    return Local<Value>();
  }
  if (internal::IsPyString(result)) {
    internal::Tagged<internal::PyString> py_string =
        internal::Tagged<internal::PyString>::cast(*result);
    return Local<Value>::Cast(WrapHostString<String>(
        isolate, std::string(py_string->buffer(),
                             static_cast<size_t>(py_string->length()))));
  }
  if (internal::IsPySmi(result)) {
    return Local<Value>::Cast(WrapHostInteger<Integer>(
        isolate, internal::PySmi::ToInt(
                     internal::Tagged<internal::PySmi>::cast(*result))));
  }
  return Local<Value>::Cast(WrapObject<RawValue>(isolate, result));
}

bool CapturePendingException(Isolate* isolate) {
  if (isolate == nullptr) {
    return false;
  }
  auto* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (internal_isolate == nullptr || !internal_isolate->HasPendingException()) {
    return false;
  }
  TryCatch* try_catch = ApiAccess::TryCatchTop(isolate);
  if (try_catch == nullptr) {
    return true;
  }
  // 约定：若存在 TryCatch，则在 API 边界消费 pending exception，
  // 并将异常对象转交给最近一层 TryCatch。
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyObject> exception =
      internal_isolate->exception_state()->pending_exception();
  ApiAccess::SetTryCatchException(
      try_catch, Local<Value>::Cast(WrapObject<RawValue>(isolate, exception)));
  internal_isolate->exception_state()->Clear();
  return true;
}

internal::MaybeHandle<internal::PyObject> InvokeEmbedderCallbackAtSlot(
    int slot_index,
    internal::Isolate* internal_isolate,
    internal::Handle<internal::PyObject> receiver,
    internal::Handle<internal::PyTuple> args,
    internal::Handle<internal::PyDict>) {
  if (slot_index < 0 || slot_index >= kMaxEmbedderCallbackSlots) {
    internal::Runtime_ThrowError(internal::ExceptionType::kRuntimeError,
                                 "Invalid embedder callback slot");
    return internal::kNullMaybeHandle;
  }
  CallbackSlot& slot = g_embedder_callback_slots[slot_index];
  if (slot.callback == nullptr || slot.isolate == nullptr) {
    internal::Runtime_ThrowError(internal::ExceptionType::kRuntimeError,
                                 "Embedder callback slot is empty");
    return internal::kNullMaybeHandle;
  }
  FunctionCallbackInfo callback_info;
  FunctionCallbackInfoImpl callback_info_impl;
  callback_info_impl.isolate = slot.isolate;
  callback_info_impl.receiver = receiver;
  callback_info_impl.args = args;
  ApiAccess::SetFunctionCallbackInfoImpl(&callback_info, &callback_info_impl);
  slot.callback(callback_info);
  if (internal_isolate->HasPendingException()) {
    return internal::kNullMaybeHandle;
  }
  return ToInternalObject(slot.isolate, callback_info_impl.return_value);
}

#define SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(SLOT)                        \
  internal::MaybeHandle<internal::PyObject> EmbedderTrampoline##SLOT(  \
      internal::Isolate* isolate,                                      \
      internal::Handle<internal::PyObject> receiver,                   \
      internal::Handle<internal::PyTuple> args,                        \
      internal::Handle<internal::PyDict> kwargs) {                     \
    return InvokeEmbedderCallbackAtSlot(SLOT, isolate, receiver, args, \
                                        kwargs);                       \
  }

SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(0)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(1)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(2)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(3)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(4)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(5)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(6)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(7)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(8)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(9)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(10)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(11)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(12)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(13)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(14)
SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE(15)

#undef SAAUSO_DEFINE_EMBEDDER_TRAMPOLINE

internal::NativeFuncPointer GetEmbedderTrampoline(int slot_index) {
  switch (slot_index) {
    case 0:
      return &EmbedderTrampoline0;
    case 1:
      return &EmbedderTrampoline1;
    case 2:
      return &EmbedderTrampoline2;
    case 3:
      return &EmbedderTrampoline3;
    case 4:
      return &EmbedderTrampoline4;
    case 5:
      return &EmbedderTrampoline5;
    case 6:
      return &EmbedderTrampoline6;
    case 7:
      return &EmbedderTrampoline7;
    case 8:
      return &EmbedderTrampoline8;
    case 9:
      return &EmbedderTrampoline9;
    case 10:
      return &EmbedderTrampoline10;
    case 11:
      return &EmbedderTrampoline11;
    case 12:
      return &EmbedderTrampoline12;
    case 13:
      return &EmbedderTrampoline13;
    case 14:
      return &EmbedderTrampoline14;
    case 15:
      return &EmbedderTrampoline15;
    default:
      return nullptr;
  }
}

}  // namespace

Isolate* Isolate::New(const IsolateCreateParams&) {
  internal::Isolate* internal_isolate = internal::Isolate::New();
  if (internal_isolate == nullptr) {
    return nullptr;
  }
  auto* isolate = new Isolate();
  isolate->internal_isolate_ = internal_isolate;
  isolate->values_ = new std::vector<Value*>();
  isolate->default_context_ = new Context(isolate);
  ApiAccess::SetValueIsolate(isolate->default_context_, isolate);
  {
    internal::Isolate::Scope isolate_scope(internal_isolate);
    internal::HandleScope handle_scope;
    internal::Handle<internal::PyDict> globals =
        internal_isolate->factory()->NewPyDict(
            internal::PyDict::kMinimumCapacity);
    ApiAccess::SetContextImpl(isolate->default_context_,
                              new ContextImpl(globals));
  }
  return isolate;
}

void Isolate::Dispose() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(this);
  auto* context_impl = reinterpret_cast<ContextImpl*>(
      ApiAccess::GetContextImpl(default_context_));
  delete context_impl;
  ApiAccess::SetContextImpl(default_context_, nullptr);
  delete default_context_;
  default_context_ = nullptr;
  if (auto* registry = ApiAccess::ValueRegistry(this); registry != nullptr) {
    for (Value* value : *registry) {
      DestroyValue(value);
    }
  }
  ApiAccess::DeleteRegisteredValues(this);
  ReleaseCallbackSlotsForIsolate(this);
  internal::Isolate::Dispose(internal_isolate);
  internal_isolate_ = nullptr;
  delete this;
}

size_t Isolate::ValueRegistrySizeForTesting() const {
  const auto* registry = ApiAccess::ValueRegistry(this);
  if (registry == nullptr) {
    return 0;
  }
  return registry->size();
}

HandleScope::HandleScope(Isolate* isolate) {
  impl_ = new HandleScopeImpl(isolate);
}

HandleScope::~HandleScope() {
  delete reinterpret_cast<HandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

Local<Context> Context::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Context>();
  }
  return Local<Context>(isolate->default_context_);
}

void Context::Enter() {
  auto* internal_isolate =
      ApiAccess::UnwrapIsolate(ApiAccess::GetValueIsolate(this));
  if (internal_isolate != nullptr) {
    internal_isolate->Enter();
  }
}

void Context::Exit() {
  auto* internal_isolate =
      ApiAccess::UnwrapIsolate(ApiAccess::GetValueIsolate(this));
  if (internal_isolate != nullptr) {
    internal_isolate->Exit();
  }
}

bool Context::Set(Local<String> key, Local<Value> value) {
  if (key.IsEmpty()) {
    return false;
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<ContextImpl*>(ApiAccess::GetContextImpl(this));
  if (internal_isolate == nullptr || context_impl == nullptr ||
      context_impl->globals.is_null()) {
    return false;
  }
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyDict> globals =
      internal::handle(context_impl->globals);
  internal::Handle<internal::PyString> py_key = internal::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  internal::Handle<internal::PyObject> py_value =
      ToInternalObject(isolate, value);
  auto maybe_set = internal::PyDict::Put(
      globals,
      internal::handle(internal::Tagged<internal::PyObject>::cast(*py_key)),
      py_value);
  if (maybe_set.IsNothing()) {
    CapturePendingException(isolate);
    return false;
  }
  return maybe_set.ToChecked();
}

MaybeLocal<Value> Context::Get(Local<String> key) {
  if (key.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<ContextImpl*>(ApiAccess::GetContextImpl(this));
  if (internal_isolate == nullptr || context_impl == nullptr ||
      context_impl->globals.is_null()) {
    return MaybeLocal<Value>();
  }
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyDict> globals =
      internal::handle(context_impl->globals);
  internal::Handle<internal::PyString> py_key = internal::PyString::NewInstance(
      key->Value().data(), static_cast<int64_t>(key->Value().size()));
  internal::Handle<internal::PyObject> out;
  auto maybe_found = globals->Get(
      internal::handle(internal::Tagged<internal::PyObject>::cast(*py_key)),
      out);
  if (maybe_found.IsNothing()) {
    CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  if (!maybe_found.ToChecked()) {
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(WrapRuntimeResult(isolate, out));
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

bool Value::IsString() const {
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == ValueKind::kHostString) {
    return true;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return false;
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return internal::IsPyString(object);
}

bool Value::IsInteger() const {
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == ValueKind::kHostInteger) {
    return true;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return false;
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null()) {
    return false;
  }
  return internal::IsPySmi(object);
}

bool Value::ToString(std::string* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == ValueKind::kHostString ||
      impl->kind == ValueKind::kScriptSource) {
    *out = impl->string_value;
    return true;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return false;
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null() || !internal::IsPyString(object)) {
    return false;
  }
  internal::Tagged<internal::PyString> py_string =
      internal::Tagged<internal::PyString>::cast(object);
  *out = std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
  return true;
}

bool Value::ToInteger(int64_t* out) const {
  if (out == nullptr) {
    return false;
  }
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return false;
  }
  if (impl->kind == ValueKind::kHostInteger) {
    *out = impl->int_value;
    return true;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return false;
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null() || !internal::IsPySmi(object)) {
    return false;
  }
  *out =
      internal::PySmi::ToInt(internal::Tagged<internal::PySmi>::cast(object));
  return true;
}

Local<String> String::New(Isolate* isolate, std::string_view value) {
  return WrapHostString<String>(isolate, std::string(value));
}

std::string String::Value() const {
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return "";
  }
  if (impl->kind == ValueKind::kHostString ||
      impl->kind == ValueKind::kScriptSource) {
    return impl->string_value;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return "";
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null() || !internal::IsPyString(object)) {
    return "";
  }
  internal::Tagged<internal::PyString> py_string =
      internal::Tagged<internal::PyString>::cast(object);
  return std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
}

Local<Integer> Integer::New(Isolate* isolate, int64_t value) {
  return WrapHostInteger<Integer>(isolate, value);
}

int64_t Integer::Value() const {
  auto* impl = GetValueImpl(this);
  if (impl == nullptr) {
    return 0;
  }
  if (impl->kind == ValueKind::kHostInteger) {
    return impl->int_value;
  }
  if (impl->kind != ValueKind::kVmObject) {
    return 0;
  }
  internal::Tagged<internal::PyObject> object = GetObjectTagged(this);
  if (object.is_null() || !internal::IsPySmi(object)) {
    return 0;
  }
  return internal::PySmi::ToInt(
      internal::Tagged<internal::PySmi>::cast(object));
}

MaybeLocal<Script> Script::Compile(Isolate* isolate, Local<String> source) {
  if (isolate == nullptr || source.IsEmpty()) {
    return MaybeLocal<Script>();
  }
  if (!Local<Value>::Cast(source)->IsString()) {
    return MaybeLocal<Script>();
  }
#if SAAUSO_ENABLE_CPYTHON_COMPILER
  return MaybeLocal<Script>(WrapScriptSource(isolate, source->Value()));
#else
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Runtime_ThrowError(
      internal::ExceptionType::kRuntimeError,
      "Script::Compile requires CPython frontend compiler support");
  CapturePendingException(isolate);
  return MaybeLocal<Script>();
#endif
}

MaybeLocal<Value> Script::Run(Local<Context> context) {
  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  auto* script_impl = GetValueImpl(this);
  if (script_impl == nullptr || script_impl->kind != ValueKind::kScriptSource) {
    return MaybeLocal<Value>();
  }
  auto* context_impl =
      reinterpret_cast<ContextImpl*>(ApiAccess::GetContextImpl(&*context));
  if (context_impl == nullptr || context_impl->globals.is_null()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyDict> globals =
      internal::handle(context_impl->globals);
  internal::MaybeHandle<internal::PyObject> maybe_result =
      internal::Runtime_ExecutePythonSourceCode(
          internal_isolate,
          std::string_view(script_impl->string_value.data(),
                           script_impl->string_value.size()),
          globals, globals);
  internal::Handle<internal::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(WrapRuntimeResult(isolate, result));
}

Local<Function> Function::New(Isolate* isolate,
                              FunctionCallback callback,
                              std::string_view name) {
  if (isolate == nullptr || callback == nullptr) {
    return Local<Function>();
  }
  int slot_index = AllocateCallbackSlot(isolate, callback);
  if (slot_index < 0) {
    internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
    internal::Isolate::Scope isolate_scope(internal_isolate);
    internal::HandleScope handle_scope;
    internal::Runtime_ThrowError(internal::ExceptionType::kRuntimeError,
                                 "No free embedder callback slots");
    CapturePendingException(isolate);
    return Local<Function>();
  }
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyString> py_name =
      internal::PyString::NewInstance(name.data(),
                                      static_cast<int64_t>(name.size()));
  internal::NativeFuncPointer native_pointer =
      GetEmbedderTrampoline(slot_index);
  internal::FunctionTemplateInfo template_info(native_pointer, py_name);
  internal::MaybeHandle<internal::PyFunction> maybe_function =
      internal_isolate->factory()->NewPyFunctionWithTemplate(template_info);
  internal::Handle<internal::PyFunction> function;
  if (!maybe_function.ToHandle(&function)) {
    CapturePendingException(isolate);
    return Local<Function>();
  }
  return WrapObject<Function>(
      isolate,
      internal::handle(internal::Tagged<internal::PyObject>::cast(*function)));
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> receiver,
                                 int argc,
                                 Local<Value> argv[]) {
  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<ContextImpl*>(ApiAccess::GetContextImpl(&*context));
  if (internal_isolate == nullptr || context_impl == nullptr) {
    return MaybeLocal<Value>();
  }
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  internal::Handle<internal::PyTuple> py_args =
      internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    internal::Handle<internal::PyObject> py_arg =
        ToInternalObject(isolate, arg);
    py_args->SetInternal(i, *py_arg);
  }
  internal::Handle<internal::PyDict> py_kwargs =
      internal_isolate->factory()->NewPyDict(
          internal::PyDict::kMinimumCapacity);
  internal::Tagged<internal::PyObject> function_object = GetObjectTagged(this);
  if (function_object.is_null()) {
    return MaybeLocal<Value>();
  }
  internal::Handle<internal::PyObject> py_receiver =
      ToInternalObject(isolate, receiver);
  internal::MaybeHandle<internal::PyObject> maybe_result =
      internal::PyObject::Call(
          internal_isolate, internal::handle(function_object), py_receiver,
          internal::handle(
              internal::Tagged<internal::PyObject>::cast(*py_args)),
          internal::handle(
              internal::Tagged<internal::PyObject>::cast(*py_kwargs)));
  internal::Handle<internal::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(WrapRuntimeResult(isolate, result));
}

int FunctionCallbackInfo::Length() const {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->args.is_null()) {
    return 0;
  }
  return static_cast<int>(impl->args->length());
}

Local<Value> FunctionCallbackInfo::operator[](int index) const {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->args.is_null() || index < 0 ||
      index >= impl->args->length()) {
    return Local<Value>();
  }
  return WrapRuntimeResult(impl->isolate, impl->args->Get(index));
}

bool FunctionCallbackInfo::GetIntegerArg(int index, int64_t* out) const {
  if (out == nullptr) {
    return false;
  }
  Local<Value> value = operator[](index);
  if (value.IsEmpty()) {
    return false;
  }
  return value->ToInteger(out);
}

bool FunctionCallbackInfo::GetStringArg(int index, std::string* out) const {
  if (out == nullptr) {
    return false;
  }
  Local<Value> value = operator[](index);
  if (value.IsEmpty()) {
    return false;
  }
  return value->ToString(out);
}

Local<Value> FunctionCallbackInfo::Receiver() const {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr) {
    return Local<Value>();
  }
  return WrapRuntimeResult(impl->isolate, impl->receiver);
}

Isolate* FunctionCallbackInfo::GetIsolate() const {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr) {
    return nullptr;
  }
  return impl->isolate;
}

void FunctionCallbackInfo::ThrowRuntimeError(std::string_view message) const {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->isolate == nullptr) {
    return;
  }
  internal::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(impl->isolate);
  if (internal_isolate == nullptr) {
    return;
  }
  internal::Isolate::Scope isolate_scope(internal_isolate);
  internal::HandleScope handle_scope;
  std::string error(message);
  internal::Runtime_ThrowError(internal::ExceptionType::kRuntimeError,
                               error.c_str());
}

void FunctionCallbackInfo::SetReturnValue(Local<Value> value) {
  auto* impl = reinterpret_cast<FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr) {
    return;
  }
  impl->return_value = value;
}

TryCatch::TryCatch(Isolate* isolate) : isolate_(isolate) {
  previous_ = ApiAccess::TryCatchTop(isolate_);
  ApiAccess::SetTryCatchTop(isolate_, this);
}

TryCatch::~TryCatch() {
  Reset();
  ApiAccess::SetTryCatchTop(isolate_, previous_);
}

bool TryCatch::HasCaught() const {
  return !exception_.IsEmpty();
}

void TryCatch::Reset() {
  exception_ = ApiAccess::MakeLocal<Value>(nullptr);
}

Local<Value> TryCatch::Exception() const {
  return exception_;
}

}  // namespace saauso
