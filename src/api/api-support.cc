#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

#include "src/api/api-impl.h"

namespace saauso {
namespace api {

namespace {

std::mutex g_embedder_callback_mutex;
std::unordered_map<int64_t, CallbackBinding> g_embedder_callback_bindings;
std::atomic<int64_t> g_next_callback_binding_id{1};

}  // namespace

int64_t RegisterCallbackBinding(Isolate* isolate, FunctionCallback callback) {
  std::lock_guard<std::mutex> guard(g_embedder_callback_mutex);
  int64_t binding_id = g_next_callback_binding_id.fetch_add(1);
  g_embedder_callback_bindings[binding_id] =
      CallbackBinding{.callback = callback, .isolate = isolate};
  return binding_id;
}

bool LookupCallbackBinding(int64_t binding_id, CallbackBinding* out) {
  std::lock_guard<std::mutex> guard(g_embedder_callback_mutex);
  auto it = g_embedder_callback_bindings.find(binding_id);
  if (it == g_embedder_callback_bindings.end()) {
    return false;
  }
  if (out != nullptr) {
    *out = it->second;
  }
  return true;
}

void ReleaseCallbackBindingsForIsolate(Isolate* isolate) {
  std::lock_guard<std::mutex> guard(g_embedder_callback_mutex);
  for (auto it = g_embedder_callback_bindings.begin();
       it != g_embedder_callback_bindings.end();) {
    if (it->second.isolate == isolate) {
      it = g_embedder_callback_bindings.erase(it);
    } else {
      ++it;
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

i::Tagged<i::PyObject> GetObjectTagged(const Value* value) {
  auto* impl = GetValueImpl(value);
  if (impl == nullptr || impl->kind != ValueKind::kVmObject) {
    return i::Tagged<i::PyObject>::null();
  }
  return impl->object;
}

i::Handle<i::PyObject> ToInternalObject(Isolate* isolate, Local<Value> value) {
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  if (value.IsEmpty()) {
    return i::handle(
        i::Tagged<i::PyObject>::cast(internal_isolate->py_none_object()));
  }
  ValueImpl* impl = GetValueImpl(&*value);
  if (impl == nullptr) {
    return i::handle(
        i::Tagged<i::PyObject>::cast(internal_isolate->py_none_object()));
  }
  if (impl->kind == ValueKind::kHostString ||
      impl->kind == ValueKind::kScriptSource) {
    i::Handle<i::PyString> py_string = i::PyString::NewInstance(
        impl->string_value.data(),
        static_cast<int64_t>(impl->string_value.size()));
    return i::handle(i::Tagged<i::PyObject>::cast(*py_string));
  }
  if (impl->kind == ValueKind::kHostInteger) {
    return i::handle(
        i::Tagged<i::PyObject>::cast(i::PySmi::FromInt(impl->int_value)));
  }
  if (impl->kind == ValueKind::kHostFloat) {
    i::Handle<i::PyFloat> py_float =
        internal_isolate->factory()->NewPyFloat(impl->float_value);
    return i::handle(i::Tagged<i::PyObject>::cast(*py_float));
  }
  if (impl->kind == ValueKind::kHostBoolean) {
    return i::handle(i::Tagged<i::PyObject>::cast(
        impl->bool_value ? internal_isolate->py_true_object()
                         : internal_isolate->py_false_object()));
  }
  if (impl->kind == ValueKind::kVmObject && !impl->object.is_null()) {
    return i::handle(impl->object);
  }
  return i::handle(
      i::Tagged<i::PyObject>::cast(internal_isolate->py_none_object()));
}

Local<Value> WrapRuntimeResult(Isolate* isolate, i::Handle<i::PyObject> result) {
  if (result.is_null()) {
    return Local<Value>();
  }
  if (i::IsPyString(result)) {
    i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*result);
    return Local<Value>::Cast(WrapHostString<String>(
        isolate, std::string(py_string->buffer(),
                             static_cast<size_t>(py_string->length()))));
  }
  if (i::IsPySmi(result)) {
    return Local<Value>::Cast(WrapHostInteger<Integer>(
        isolate, i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*result))));
  }
  if (i::IsPyFloat(result)) {
    i::Tagged<i::PyFloat> py_float = i::Tagged<i::PyFloat>::cast(*result);
    return Local<Value>::Cast(WrapHostFloat<Float>(isolate, py_float->value()));
  }
  if (i::IsPyTrue(result)) {
    return Local<Value>::Cast(WrapHostBoolean<Boolean>(isolate, true));
  }
  if (i::IsPyFalse(result)) {
    return Local<Value>::Cast(WrapHostBoolean<Boolean>(isolate, false));
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
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyObject> exception =
      internal_isolate->exception_state()->pending_exception();
  ApiAccess::SetTryCatchException(
      try_catch, Local<Value>::Cast(WrapObject<RawValue>(isolate, exception)));
  internal_isolate->exception_state()->Clear();
  return true;
}

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict>,
    i::Handle<i::PyObject> closure_data) {
  if (closure_data.is_null() || !i::IsPySmi(closure_data)) {
    i::Runtime_ThrowError(i::ExceptionType::kRuntimeError,
                          "Embedder callback closure_data is invalid");
    return i::kNullMaybeHandle;
  }
  int64_t binding_id =
      i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*closure_data));
  CallbackBinding binding;
  if (!LookupCallbackBinding(binding_id, &binding) ||
      binding.callback == nullptr || binding.isolate == nullptr) {
    i::Runtime_ThrowError(i::ExceptionType::kRuntimeError,
                          "Embedder callback binding is missing");
    return i::kNullMaybeHandle;
  }
  EscapableHandleScope escapable_scope(binding.isolate);
  FunctionCallbackInfo callback_info;
  FunctionCallbackInfoImpl callback_info_impl;
  callback_info_impl.isolate = binding.isolate;
  callback_info_impl.receiver = receiver;
  callback_info_impl.args = args;
  ApiAccess::SetFunctionCallbackInfoImpl(&callback_info, &callback_info_impl);
  binding.callback(callback_info);
  if (internal_isolate->HasPendingException()) {
    return i::kNullMaybeHandle;
  }
  return ToInternalObject(binding.isolate, callback_info_impl.return_value);
}

}  // namespace api
}  // namespace saauso
