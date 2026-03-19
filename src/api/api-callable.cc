#include <string>

#include "src/api/api-impl.h"

namespace saauso {

Local<Function> Function::New(Isolate* isolate,
                              FunctionCallback callback,
                              std::string_view name) {
  if (isolate == nullptr || callback == nullptr) {
    return Local<Function>();
  }
  int64_t binding_id = api::RegisterCallbackBinding(isolate, callback);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyString> py_name =
      i::PyString::NewInstance(name.data(), static_cast<int64_t>(name.size()));
  i::Handle<i::PyObject> closure_data =
      i::handle(i::Tagged<i::PyObject>::cast(i::PySmi::FromInt(binding_id)));
  i::FunctionTemplateInfo template_info(&api::InvokeEmbedderCallback, py_name,
                                        closure_data);
  i::MaybeHandle<i::PyFunction> maybe_function =
      internal_isolate->factory()->NewPyFunctionWithTemplate(template_info);
  i::Handle<i::PyFunction> function;
  if (!maybe_function.ToHandle(&function)) {
    api::CapturePendingException(isolate);
    return Local<Function>();
  }
  return api::WrapObject<Function>(
      isolate, i::handle(i::Tagged<i::PyObject>::cast(*function)));
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> receiver,
                                 int argc,
                                 Local<Value> argv[]) {
  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(this);
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(isolate);
  auto* context_impl =
      reinterpret_cast<api::ContextImpl*>(ApiAccess::GetContextImpl(&*context));
  if (internal_isolate == nullptr || context_impl == nullptr) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  i::Handle<i::PyTuple> py_args = internal_isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    i::Handle<i::PyObject> py_arg = api::ToInternalObject(isolate, arg);
    py_args->SetInternal(i, *py_arg);
  }
  i::Handle<i::PyDict> py_kwargs =
      internal_isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Tagged<i::PyObject> function_object = api::GetObjectTagged(this);
  if (function_object.is_null()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> py_receiver =
      receiver.IsEmpty() ? i::Handle<i::PyObject>::null()
                         : api::ToInternalObject(isolate, receiver);
  i::MaybeHandle<i::PyObject> maybe_result = i::PyObject::Call(
      internal_isolate, i::handle(function_object), py_receiver,
      i::handle(i::Tagged<i::PyObject>::cast(*py_args)),
      i::handle(i::Tagged<i::PyObject>::cast(*py_kwargs)));
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  return MaybeLocal<Value>(api::WrapRuntimeResult(isolate, result));
}

Local<Value> Exception::TypeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(&*msg);
  if (isolate == nullptr) {
    return Local<Value>();
  }
  return Local<Value>::Cast(
      String::New(isolate, "[TypeError] " + msg->Value()));
}

Local<Value> Exception::RuntimeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }
  Isolate* isolate = ApiAccess::GetValueIsolate(&*msg);
  if (isolate == nullptr) {
    return Local<Value>();
  }
  return Local<Value>::Cast(
      String::New(isolate, "[RuntimeError] " + msg->Value()));
}

int FunctionCallbackInfo::Length() const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->args.is_null()) {
    return 0;
  }
  return static_cast<int>(impl->args->length());
}

Local<Value> FunctionCallbackInfo::operator[](int index) const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->args.is_null() || index < 0 ||
      index >= impl->args->length()) {
    return Local<Value>();
  }
  return api::WrapRuntimeResult(impl->isolate, impl->args->Get(index));
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
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr) {
    return Local<Value>();
  }
  return api::WrapRuntimeResult(impl->isolate, impl->receiver);
}

Isolate* FunctionCallbackInfo::GetIsolate() const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr) {
    return nullptr;
  }
  return impl->isolate;
}

void FunctionCallbackInfo::ThrowRuntimeError(std::string_view message) const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->isolate == nullptr) {
    return;
  }
  i::Isolate* internal_isolate = ApiAccess::UnwrapIsolate(impl->isolate);
  if (internal_isolate == nullptr) {
    return;
  }
  i::Isolate::Scope isolate_scope(internal_isolate);
  i::HandleScope handle_scope;
  std::string error(message);
  i::Runtime_ThrowError(i::ExceptionType::kRuntimeError, error.c_str());
}

void FunctionCallbackInfo::SetReturnValue(Local<Value> value) {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
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
