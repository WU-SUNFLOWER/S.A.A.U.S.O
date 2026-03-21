#include <string>

#include "include/saauso-function.h"
#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {

Local<Function> Function::New(Isolate* isolate,
                              FunctionCallback callback,
                              std::string_view name) {
  auto* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
  i::Isolate::Scope isolate_scope(internal_isolate);

  if (isolate == nullptr || callback == nullptr) {
    return Local<Function>();
  }

  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyString> py_name =
      i::PyString::NewInstance(name.data(), static_cast<int64_t>(name.size()));
  int64_t callback_addr =
      static_cast<int64_t>(reinterpret_cast<intptr_t>(callback));
  i::Handle<i::PyObject> closure_data =
      i::handle(i::Tagged<i::PyObject>::cast(i::PySmi::FromInt(callback_addr)));
  i::FunctionTemplateInfo template_info(&api::InvokeEmbedderCallback, py_name,
                                        closure_data);
  i::MaybeHandle<i::PyFunction> maybe_function =
      internal_isolate->factory()->NewPyFunctionWithTemplate(template_info);
  i::Handle<i::PyFunction> function;
  if (!maybe_function.ToHandle(&function)) {
    api::CapturePendingException(internal_isolate);
    return Local<Function>();
  }
  i::Handle<i::PyObject> escaped =
      handle_scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*function)));
  return internal::Utils::ToLocal<Function>(escaped);
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> receiver,
                                 int argc,
                                 Local<Value> argv[]) {
  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Isolate* isolate = i::Isolate::Current();

  i::Handle<i::PyObject> context_object = internal::Utils::OpenHandle(context);
  if (isolate == nullptr || context_object.is_null() ||
      !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }
  i::Isolate::Scope isolate_scope(isolate);
  i::EscapableHandleScope handle_scope;
  i::Handle<i::PyTuple> py_args = isolate->factory()->NewPyTuple(argc);
  for (int i = 0; i < argc; ++i) {
    Local<Value> arg = argv == nullptr ? Local<Value>() : argv[i];
    i::Handle<i::PyObject> py_arg = api::ToInternalObject(isolate, arg);
    py_args->SetInternal(i, *py_arg);
  }
  i::Handle<i::PyDict> py_kwargs =
      isolate->factory()->NewPyDict(i::PyDict::kMinimumCapacity);
  i::Handle<i::PyObject> function_object = internal::Utils::OpenHandle(this);
  if (function_object.is_null()) {
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> py_receiver =
      receiver.IsEmpty() ? i::Handle<i::PyObject>::null()
                         : api::ToInternalObject(isolate, receiver);
  i::MaybeHandle<i::PyObject> maybe_result =
      i::PyObject::Call(isolate, function_object, py_receiver,
                        i::handle(i::Tagged<i::PyObject>::cast(*py_args)),
                        i::handle(i::Tagged<i::PyObject>::cast(*py_kwargs)));
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return MaybeLocal<Value>(internal::Utils::ToLocal<Value>(escaped));
}

Local<Value> Exception::TypeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }

  i::Isolate* isolate = i::Isolate::Current();
  return Local<Value>::Cast(String::New(reinterpret_cast<Isolate*>(isolate),
                                        "[TypeError] " + msg->Value()));
}

Local<Value> Exception::RuntimeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }

  i::Isolate* isolate = i::Isolate::Current();
  return Local<Value>::Cast(String::New(reinterpret_cast<Isolate*>(isolate),
                                        "[RuntimeError] " + msg->Value()));
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
  return reinterpret_cast<Isolate*>(impl->isolate);
}

void FunctionCallbackInfo::ThrowRuntimeError(std::string_view message) const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(
      ApiAccess::GetFunctionCallbackInfoImpl(this));
  if (impl == nullptr || impl->isolate == nullptr) {
    return;
  }

  auto* internal_isolate = reinterpret_cast<i::Isolate*>(impl->isolate);
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
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  previous_ = i_isolate->try_catch_top();
  i_isolate->set_try_catch_top(this);
}

TryCatch::~TryCatch() {
  Reset();

  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  i_isolate->set_try_catch_top(previous_);
}

bool TryCatch::HasCaught() const {
  return !exception_.IsEmpty();
}

void TryCatch::Reset() {
  exception_ = Local<Value>();
}

Local<Value> TryCatch::Exception() const {
  return exception_;
}

}  // namespace saauso
