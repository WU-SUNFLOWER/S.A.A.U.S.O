#ifndef SAAUSO_API_API_IMPL_H_
#define SAAUSO_API_API_IMPL_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "include/saauso-container.h"
#include "include/saauso-exception.h"
#include "include/saauso-local-handle.h"
#include "include/saauso-object.h"
#include "include/saauso-script.h"
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
    return Local<T>::FromSlot(handle.location());
  }
};
}  // namespace internal

namespace api {

class RawValue final : public Value {};
class RawObject final : public Object {};
class RawList final : public List {};
class RawTuple final : public Tuple {};

struct FunctionCallbackInfoImpl {
  i::Isolate* isolate{nullptr};
  i::Handle<i::PyObject> receiver;
  i::Handle<i::PyTuple> args;
  Local<Value> return_value;
};

i::Handle<i::PyObject> ToInternalObject(i::Isolate* isolate,
                                        Local<Value> value);
Local<Value> WrapRuntimeResult(i::Isolate* isolate,
                               i::Handle<i::PyObject> result);
bool CapturePendingException(i::Isolate* isolate);

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict> kwargs,
    i::Handle<i::PyObject> closure_data);
#if SAAUSO_ENABLE_CPYTHON_COMPILER
Local<Script> WrapScriptSource(i::Isolate* isolate, std::string source);
#endif

template <typename T>
Local<T> WrapObject(i::Isolate* isolate, i::Handle<i::PyObject> object) {
  return i::Utils::ToLocal<T>(object);
}

template <typename T>
Local<T> WrapHostString(i::Isolate* isolate, std::string value) {
  i::EscapableHandleScope scope(isolate);
  i::Handle<i::PyString> py_string = i::PyString::New(
      isolate, value.data(), static_cast<int64_t>(value.size()));
  i::Handle<i::PyObject> escaped = scope.Escape(py_string);
  return i::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostInteger(i::Isolate* isolate, int64_t value) {
  i::EscapableHandleScope scope(isolate);
  i::Handle<i::PyObject> smi = isolate->factory()->NewSmiFromInt(value);
  i::Handle<i::PyObject> escaped = scope.Escape(smi);
  return i::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostFloat(i::Isolate* isolate, double value) {
  i::EscapableHandleScope scope(isolate);
  i::Handle<i::PyFloat> py_float = isolate->factory()->NewPyFloat(value);
  i::Handle<i::PyObject> escaped = scope.Escape(py_float);
  return i::Utils::ToLocal<T>(escaped);
}

template <typename T>
Local<T> WrapHostBoolean(i::Isolate* isolate, bool value) {
  i::EscapableHandleScope scope(isolate);
  i::Handle<i::PyObject> py_bool = isolate->factory()->ToPyBoolean(value);
  i::Handle<i::PyObject> escaped = scope.Escape(py_bool);
  return i::Utils::ToLocal<T>(escaped);
}

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_IMPL_H_
