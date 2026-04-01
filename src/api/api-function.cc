#include <string>

#include "include/saauso-function.h"
#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {

MaybeLocal<Function> Function::New(Isolate* isolate,
                                   FunctionCallback callback,
                                   std::string_view name) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  i::EscapableHandleScope handle_scope(i_isolate);
  i::Handle<i::PyString> py_name = i::PyString::New(
      i_isolate, name.data(), static_cast<int64_t>(name.size()));
  int64_t callback_addr =
      static_cast<int64_t>(reinterpret_cast<intptr_t>(callback));
  i::Handle<i::PyObject> closure_data =
      i_isolate->factory()->NewSmiFromInt(callback_addr);
  i::FunctionTemplateInfo template_info(i_isolate, &api::InvokeEmbedderCallback,
                                        py_name, closure_data);
  i::MaybeHandle<i::PyFunction> maybe_function =
      i_isolate->factory()->NewPyFunctionWithTemplate(template_info);
  i::Handle<i::PyFunction> function;
  if (!maybe_function.ToHandle(&function)) {
    api::CapturePendingException(i_isolate);
    return MaybeLocal<Function>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(function);
  return i::Utils::ToLocal<Function>(escaped);
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> receiver,
                                 int argc,
                                 Local<Value> argv[]) {
  auto* i_isolate = i::Isolate::Current();
  assert(i_isolate != nullptr);

  i::EscapableHandleScope handle_scope(i_isolate);

  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(context);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyTuple> py_args;
  if (argc > 0) {
    assert(argv != nullptr);
    py_args = i_isolate->factory()->NewPyTuple(argc);
    for (int i = 0; i < argc; ++i) {
      i::Handle<i::PyObject> py_arg = i::Utils::OpenHandle(argv[i]);
      assert(!py_arg.is_null());
      py_args->SetInternal(i, *py_arg);
    }
  }

  i::Handle<i::PyObject> function_object = internal::Utils::OpenHandle(this);
  if (function_object.is_null()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> py_receiver = receiver.IsEmpty()
                                           ? i::Handle<i::PyObject>::null()
                                           : i::Utils::OpenHandle(receiver);

  i::MaybeHandle<i::PyObject> maybe_result =
      i::PyObject::Call(i_isolate, function_object, py_receiver, py_args,
                        i::Handle<i::PyObject>::null());
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(i_isolate);
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return i::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
