// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception-klass.h"

#include "include/saauso-internal.h"
#include "src/builtins/builtins-base-exception-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/py-base-exception.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// static
Tagged<PyBaseExceptionKlass> PyBaseExceptionKlass::GetInstance(
    Isolate* isolate) {
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

  // TODO:
  // BaseException 实例当前处于“布局字段 + __dict__”并存阶段。
  // 未来需要移除 __dict__ ，让架构进一步与 CPython 对齐。
  set_instance_has_properties_dict(true);
  set_native_layout_kind(NativeLayoutKind::kBaseException);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

  vtable_.Clear();
  vtable_.init_instance_ = &Virtual_InitInstance;
  vtable_.str_ = &Virtual_Str;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyBaseExceptionKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  Handle<PyDict> klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate,
                      BaseExceptionMethods::Install(isolate, klass_properties,
                                                    type_object(isolate)));

  // 设置类名
  set_name(PyString::New(isolate, "BaseException"));

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
  Tagged<Klass> instance_klass =
      PyObject::ResolveObjectKlass(instance, isolate);

  bool is_valid_klass = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_valid_klass,
      Runtime_IsSubtype(isolate, instance_klass,
                        PyBaseExceptionKlass::GetInstance(isolate)));
  if (!is_valid_klass) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "descriptor '__init__' requires a 'BaseException' object but "
        "received a '%s'",
        PyObject::GetTypeName(instance, isolate)->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyDict> dict_kwargs = Handle<PyDict>::cast(kwargs);
  if (!dict_kwargs.is_null() && dict_kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "BaseException.__init__() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> init_args =
      args.is_null() ? PyTuple::New(isolate, 0) : Handle<PyTuple>::cast(args);
  PyBaseException::cast(*instance)->set_args(init_args);

  return isolate->factory()->py_none_object();
}

MaybeHandle<PyObject> PyBaseExceptionKlass::Virtual_Repr(
    Isolate* isolate,
    Handle<PyObject> self) {
  EscapableHandleScope scope(isolate);

  Handle<PyString> type_name = PyObject::GetTypeName(self, isolate);
  Handle<PyTuple> exception_args = PyBaseException::cast(*self)->args(isolate);
  if (exception_args.is_null()) {
    exception_args = PyTuple::New(isolate, 0);
  }
  Handle<PyString> args_repr;
  if (exception_args->length() == 1) {
    Handle<PyObject> arg_repr_obj;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, arg_repr_obj,
        PyObject::Repr(isolate, exception_args->Get(0, isolate)));
    Handle<PyString> arg_repr = Handle<PyString>::cast(arg_repr_obj);
    args_repr = PyString::New(isolate, "(");
    args_repr = PyString::Append(args_repr, arg_repr, isolate);
    args_repr =
        PyString::Append(args_repr, PyString::New(isolate, ")"), isolate);
  } else {
    Handle<PyObject> args_repr_obj;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, args_repr_obj,
        PyObject::Repr(isolate, Handle<PyObject>::cast(exception_args)));
    args_repr = Handle<PyString>::cast(args_repr_obj);
  }

  Handle<PyString> result = PyString::Append(type_name, args_repr, isolate);

  return scope.Escape(result);
}

MaybeHandle<PyObject> PyBaseExceptionKlass::Virtual_Str(Isolate* isolate,
                                                        Handle<PyObject> self) {
  EscapableHandleScope scope(isolate);

  Handle<PyString> args_message;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, args_message,
                             Runtime_ParseExceptionMessageFromArgs(
                                 isolate, Handle<PyBaseException>::cast(self)));

  return scope.Escape(args_message);
}

size_t PyBaseExceptionKlass::Virtual_InstanceSize(Tagged<PyObject>) {
  return ObjectSizeAlign(sizeof(PyBaseException));
}

void PyBaseExceptionKlass::Virtual_Iterate(Tagged<PyObject> self,
                                           ObjectVisitor* v) {
  Tagged<PyBaseException> exception = PyBaseException::cast(self);
  v->VisitPointer(&exception->args_);
}

}  // namespace saauso::internal
