#ifndef SAAUSO_API_API_IMPL_H_
#define SAAUSO_API_API_IMPL_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "include/saauso-embedder.h"
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
namespace api {

class RawValue final : public Value {};
class RawObject final : public Object {};
class RawList final : public List {};
class RawTuple final : public Tuple {};

enum class ValueKind {
  kNone,
  kHostString,
  kHostInteger,
  kHostFloat,
  kHostBoolean,
  kScriptSource,
  kVmObject,
};

struct ValueImpl {
  ValueKind kind{ValueKind::kNone};
  std::string string_value;
  int64_t int_value{0};
  double float_value{0.0};
  bool bool_value{false};
  i::Tagged<i::PyObject> object{i::Tagged<i::PyObject>::null()};
};

struct ContextImpl {
  explicit ContextImpl(i::Handle<i::PyDict> globals)
      : globals(globals.is_null() ? i::Tagged<i::PyDict>::null() : *globals) {}

  i::Tagged<i::PyDict> globals;
};

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
void DestroyValue(Value* value);
ValueImpl* GetValueImpl(const Value* value);
i::Tagged<i::PyObject> GetObjectTagged(const Value* value);
i::Handle<i::PyObject> ToInternalObject(Isolate* isolate, Local<Value> value);
Local<Value> WrapRuntimeResult(Isolate* isolate, i::Handle<i::PyObject> result);
bool CapturePendingException(Isolate* isolate);
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

template <typename T>
Local<T> WrapHostFloat(Isolate* isolate, double value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  auto* obj = new T();
  ApiAccess::RegisterValue(isolate, obj);
  ApiAccess::SetValueIsolate(obj, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kHostFloat;
  impl->float_value = value;
  ApiAccess::SetValueImpl(obj, impl);
  return ApiAccess::MakeLocal(obj);
}

template <typename T>
Local<T> WrapHostBoolean(Isolate* isolate, bool value) {
  if (isolate == nullptr) {
    return Local<T>();
  }
  auto* obj = new T();
  ApiAccess::RegisterValue(isolate, obj);
  ApiAccess::SetValueIsolate(obj, isolate);
  auto* impl = new ValueImpl();
  impl->kind = ValueKind::kHostBoolean;
  impl->bool_value = value;
  ApiAccess::SetValueImpl(obj, impl);
  return ApiAccess::MakeLocal(obj);
}

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_IMPL_H_
