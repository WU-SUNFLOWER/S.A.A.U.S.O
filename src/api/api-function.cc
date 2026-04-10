#include <string>

#include "include/saauso-function.h"
#include "include/saauso-primitive.h"
#include "src/api/api-exception-support.h"
#include "src/api/api-function-callback-support.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/heap/factory.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/templates.h"

namespace saauso {

Local<Function> Function::New(Isolate* isolate,
                              FunctionCallback callback,
                              std::string_view name) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyString> py_name = i::PyString::New(
      i_isolate, name.data(), static_cast<int64_t>(name.size()));
  int64_t callback_addr =
      static_cast<int64_t>(reinterpret_cast<intptr_t>(callback));
  i::Handle<i::PyObject> closure_data =
      i_isolate->factory()->NewSmiFromInt(callback_addr);
  i::FunctionTemplateInfo template_info(i_isolate, &api::InvokeEmbedderCallback,
                                        py_name, closure_data);

  i::Handle<i::PyFunction> function =
      i_isolate->factory()->NewPyFunctionWithTemplate(template_info);

  i::Handle<i::PyObject> escaped = handle_scope.Escape(function);
  return api::Utils::ToLocal<Function>(escaped);
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> receiver,
                                 int argc,
                                 Local<Value> argv[]) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  i::EscapableHandleScope handle_scope(i_isolate);

  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(context);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyTuple> py_args;
  if (argc > 0) {
    assert(argv != nullptr);
    py_args = i_isolate->factory()->NewPyTuple(argc);
    for (int i = 0; i < argc; ++i) {
      i::Handle<i::PyObject> py_arg = api::Utils::OpenHandle(argv[i]);
      assert(!py_arg.is_null());
      py_args->SetInternal(i, *py_arg);
    }
  }

  i::Handle<i::PyObject> function_object = api::Utils::OpenHandle(this);
  if (function_object.is_null()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> py_receiver = receiver.IsEmpty()
                                           ? i::Handle<i::PyObject>::null()
                                           : api::Utils::OpenHandle(receiver);

  i::MaybeHandle<i::PyObject> maybe_result =
      i::PyObject::Call(i_isolate, function_object, py_receiver, py_args,
                        i::Handle<i::PyObject>::null());
  return api::ToLocalOrCapturePendingException<Value>(i_isolate, handle_scope,
                                                      maybe_result);
}

}  // namespace saauso
