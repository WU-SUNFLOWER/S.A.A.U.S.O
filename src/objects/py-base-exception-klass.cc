// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception-klass.h"

#include "include/saauso-internal.h"
#include "src/builtins/builtins-base-exception-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
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

  Handle<PyObject> str_method;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, str_method,
      Runtime_GetPropertyInKlassMro(isolate, instance_klass, ST(str)));
  Handle<PyObject> message_obj;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, message_obj,
      Execution::Call(isolate, str_method, instance, PyTuple::NewInstance(0),
                      Handle<PyDict>::null()));
  if (!IsPyString(message_obj)) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() returned non-string");
    return kNullMaybeHandle;
  }

  RETURN_ON_EXCEPTION_VALUE(isolate,
                            PyDict::Put(properties, ST(message), message_obj),
                            kNullMaybeHandle);

  return handle(isolate->py_none_object());
}

}  // namespace saauso::internal
