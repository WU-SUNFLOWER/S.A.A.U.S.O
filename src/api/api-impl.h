#ifndef SAAUSO_API_API_IMPL_H_
#define SAAUSO_API_API_IMPL_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "include/saauso-container.h"
#include "include/saauso-exception.h"
#include "include/saauso-local-handle.h"
#include "include/saauso-object.h"
#include "include/saauso-value.h"
#include "src/api/api-inl.h"
#include "src/common/globals.h"
#include "src/execution/exception-types.h"
#include "src/handles/handle-scopes.h"
#include "src/heap/factory.h"
#include "src/objects/object-checkers.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/templates.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-exec.h"
#include "src/runtime/runtime-py-function.h"

namespace saauso {

namespace internal {
class Utils {
 public:
  template <typename T>
  static inline i::Handle<i::PyObject> OpenHandle(Local<T> local) {
    return OpenHandle(*local);
  }

  template <typename T>
  static inline i::Handle<i::PyObject> OpenHandle(const T* value) {
    if (value == nullptr) {
      return i::Handle<i::PyObject>::null();
    }
    return i::Handle<i::PyObject>(
        reinterpret_cast<i::Address*>(const_cast<T*>(value)));
  }

  template <typename T>
  static inline Local<T> ToLocal(i::Handle<i::PyObject> handle) {
    if (handle.is_null()) {
      return Local<T>();
    }
    return Local<T>(reinterpret_cast<T*>(handle.location()));
  }
};
}  // namespace internal

namespace api {

class RawValue final : public Value {};
class RawObject final : public Object {};
class RawList final : public List {};
class RawTuple final : public Tuple {};

struct FunctionCallbackInfoImpl {
  Isolate* isolate{nullptr};
  i::Handle<i::PyObject> receiver;
  i::Handle<i::PyTuple> args;
  Local<Value> return_value;
};

struct CallbackBinding {
  FunctionCallback callback{nullptr};
  Isolate* isolate{nullptr};
};

int64_t RegisterCallbackBinding(Isolate* isolate, FunctionCallback callback);
void ReleaseCallbackBindingsForIsolate(Isolate* isolate);
i::Handle<i::PyObject> ToInternalObject(Isolate* isolate, Local<Value> value);
Local<Value> WrapRuntimeResult(Isolate* isolate, i::Handle<i::PyObject> result);
bool CapturePendingException(Isolate* isolate);
void RegisterPublicIsolate(Isolate* isolate);
void UnregisterPublicIsolate(Isolate* isolate);
Isolate* CurrentPublicIsolate();
i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict> kwargs,
    i::Handle<i::PyObject> closure_data);
#if SAAUSO_ENABLE_CPYTHON_COMPILER
Local<Script> WrapScriptSource(Isolate* isolate, std::string source);
#endif

template <typename T>
Local<T> WrapObject(Isolate* isolate, i::Handle<i::PyObject> object) {
  if (isolate == nullptr || object.is_null()) {
    return Local<T>();
  }
  return internal::Utils::ToLocal<T>(object);
}

template <typename T>
Local<T> WrapHostString(Isolate* isolate, std::string value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope scope;
  i::Handle<i::PyString> py_string = i::PyString::NewInstance(
      value.data(), static_cast<int64_t>(value.size()));
  i::Handle<i::PyObject> escaped =
      scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*py_string)));
  return internal::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostInteger(Isolate* isolate, int64_t value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope scope;
  i::Handle<i::PyObject> smi =
      i::handle(i::Tagged<i::PyObject>::cast(i::PySmi::FromInt(value)));
  i::Handle<i::PyObject> escaped = scope.Escape(smi);
  return internal::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostFloat(Isolate* isolate, double value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope scope;
  i::Handle<i::PyFloat> py_float =
      internal_isolate->factory()->NewPyFloat(value);
  i::Handle<i::PyObject> escaped =
      scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*py_float)));
  return internal::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostBoolean(Isolate* isolate, bool value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::EscapableHandleScope scope;
  i::Handle<i::PyObject> py_bool = i::handle(i::Tagged<i::PyObject>::cast(
      value ? internal_isolate->py_true_object()
            : internal_isolate->py_false_object()));
  i::Handle<i::PyObject> escaped = scope.Escape(py_bool);
  return internal::Utils::ToLocal<T>(escaped);
}

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_IMPL_H_
