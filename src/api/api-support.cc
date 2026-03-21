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

#if SAAUSO_ENABLE_CPYTHON_COMPILER
Local<Script> WrapScriptSource(Isolate* isolate, std::string source) {
  return WrapHostString<Script>(reinterpret_cast<i::Isolate*>(isolate),
                                std::move(source));
}
#endif

i::Handle<i::PyObject> ToInternalObject(i::Isolate* internal_isolate,
                                        Local<Value> value) {
  if (value.IsEmpty()) {
    return i::handle(
        i::Tagged<i::PyObject>::cast(internal_isolate->py_none_object()));
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(value);
  if (object.is_null()) {
    return i::handle(
        i::Tagged<i::PyObject>::cast(internal_isolate->py_none_object()));
  }
  return object;
}

Local<Value> WrapRuntimeResult(Isolate* isolate,
                               i::Handle<i::PyObject> result) {
  if (result.is_null()) {
    return Local<Value>();
  }
  return Local<Value>::Cast(
      WrapObject<RawValue>(reinterpret_cast<i::Isolate*>(isolate), result));
}

bool CapturePendingException(i::Isolate* isolate) {
  if (!isolate->HasPendingException()) {
    return false;
  }

  TryCatch* try_catch = isolate->try_catch_top();
  if (try_catch == nullptr) {
    return true;
  }

  i::Isolate::Scope isolate_scope(isolate);
  i::Handle<i::PyObject> exception =
      isolate->exception_state()->pending_exception();
  ApiAccess::SetTryCatchException(
      try_catch, Local<Value>::Cast(WrapObject<RawValue>(isolate, exception)));
  isolate->exception_state()->Clear();
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
  Local<Value> escaped_ret =
      escapable_scope.Escape(callback_info_impl.return_value);
  return ToInternalObject(reinterpret_cast<i::Isolate*>(binding.isolate),
                          escaped_ret);
}

}  // namespace api
}  // namespace saauso
