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

struct FunctionCallbackInfoImpl {
  i::Isolate* isolate{nullptr};
  i::Handle<i::PyObject> receiver;
  i::Handle<i::PyTuple> args;
  Local<Value> return_value;
};

bool CapturePendingException(i::Isolate* isolate);

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict> kwargs,
    i::Handle<i::PyObject> closure_data);

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_IMPL_H_
