// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception-klass.h"

#include "include/saauso-internal.h"
#include "src/builtins/builtins-base-exception-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

MaybeHandle<PyString> MessageFromArgsTuple(Handle<PyTuple> exception_args) {
  if (exception_args.is_null() || exception_args->length() == 0) {
    return PyString::NewInstance("");
  }
  if (exception_args->length() == 1 && IsPyString(exception_args->Get(0))) {
    return Handle<PyString>::cast(exception_args->Get(0));
  }
  return PyString::NewInstance("");
}

MaybeHandle<PyTuple> ReadExceptionArgs(Isolate* isolate,
                                       Handle<PyObject> self) {
  Handle<PyDict> properties = PyObject::GetProperties(self);
  if (properties.is_null()) {
    return Handle<PyTuple>::null();
  }

  Handle<PyObject> args_obj;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             properties->Get(ST(args), args_obj));
  if (!found || !IsPyTuple(args_obj)) {
    return Handle<PyTuple>::null();
  }
  return Handle<PyTuple>::cast(args_obj);
}

}  // namespace

// static
Tagged<PyBaseExceptionKlass> PyBaseExceptionKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyBaseExceptionKlass> instance = isolate->py_base_exception_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyBaseExceptionKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_base_exception_klass(instance);
  }
  return instance;
}

void PyBaseExceptionKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // BaseException 实例目前仍采用 __dict__ 存储语义字段（args/message 等）。
  set_instance_has_properties_dict(true);
  set_native_layout_kind(NativeLayoutKind::kPyObject);
  set_native_layout_base(PyObjectKlass::GetInstance());

  vtable_.Clear();
  vtable_.init_instance_ = &Virtual_InitInstance;
  vtable_.str_ = &Virtual_Str;
  vtable_.repr_ = &Virtual_Repr;
}

Maybe<void> PyBaseExceptionKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  Handle<PyDict> klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate, BaseExceptionMethods::Install(
                                   isolate, klass_properties, type_object()));

  // 设置类名
  set_name(PyString::NewInstance("BaseException"));

  return JustVoid();
}

void PyBaseExceptionKlass::Finalize(Isolate* isolate) {
  isolate->set_py_base_exception_klass(Tagged<PyBaseExceptionKlass>::null());
}

MaybeHandle<PyObject> PyBaseExceptionKlass::Virtual_InitInstance(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> instance_klass = PyObject::GetKlass(instance);

  bool is_valid_klass = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_valid_klass,
      Runtime_IsSubtype(instance_klass, PyBaseExceptionKlass::GetInstance()));
  if (!is_valid_klass) [[unlikely]] {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "descriptor '__init__' requires a 'BaseException' object but "
        "received a '%s'",
        instance_klass->name()->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyDict> dict_kwargs = Handle<PyDict>::cast(kwargs);
  if (!dict_kwargs.is_null() && dict_kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__init__() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> init_args =
      args.is_null() ? PyTuple::NewInstance(0) : Handle<PyTuple>::cast(args);
  Handle<PyDict> properties = PyObject::GetProperties(instance);
  if (properties.is_null()) {
    properties = PyDict::NewInstance();
    PyObject::SetProperties(*instance, *properties);
  }

  RETURN_ON_EXCEPTION_VALUE(
      isolate, PyDict::Put(properties, ST(args), init_args), kNullMaybeHandle);

  Handle<PyString> message;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, message, MessageFromArgsTuple(init_args));
  RETURN_ON_EXCEPTION_VALUE(
      isolate, PyDict::Put(properties, ST(message), message), kNullMaybeHandle);

  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> PyBaseExceptionKlass::Virtual_Str(Handle<PyObject> self) {
  auto* isolate = Isolate::Current();
  EscapableHandleScope scope;

  Handle<PyTuple> exception_args;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, exception_args,
                             ReadExceptionArgs(isolate, self));
  if (!exception_args.is_null()) {
    Handle<PyString> args_message;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, args_message,
                               MessageFromArgsTuple(exception_args));
    return scope.Escape(args_message);
  }

  Handle<PyDict> properties = PyObject::GetProperties(self);
  if (!properties.is_null()) {
    Handle<PyObject> message;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                               properties->Get(ST(message), message));
    if (found && IsPyString(message)) {
      return scope.Escape(Handle<PyString>::cast(message));
    }
  }

  return scope.Escape(PyString::NewInstance(""));
}

MaybeHandle<PyObject> PyBaseExceptionKlass::Virtual_Repr(
    Handle<PyObject> self) {
  auto* isolate = Isolate::Current();
  EscapableHandleScope scope;

  Handle<PyString> type_name = PyObject::GetKlass(self)->name();
  Handle<PyObject> message_obj;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, message_obj, Virtual_Str(self));
  Handle<PyString> message = Handle<PyString>::cast(message_obj);

  Handle<PyString> result = PyString::NewInstance("<");
  result = PyString::Append(result, type_name);
  if (!message.is_null() && !message->IsEmpty()) {
    result = PyString::Append(result, PyString::NewInstance(": "));
    result = PyString::Append(result, message);
  }
  result = PyString::Append(result, PyString::NewInstance(">"));

  return scope.Escape(result);
}

}  // namespace saauso::internal
