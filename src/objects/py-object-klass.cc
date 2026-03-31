// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object-klass.h"

#include "src/builtins/accessor-proxy.h"
#include "src/builtins/builtins-py-object-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

// static
Tagged<PyObjectKlass> PyObjectKlass::GetInstance(Isolate* isolate) {
  Tagged<PyObjectKlass> instance = isolate->py_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_object_klass(instance);
  }
  return instance;
}

///////////////////////////////////////////////////////////////////////////////

void PyObjectKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.hash_ = &Generic_Hash;
  vtable_.getattr_ = &Generic_GetAttr;
  vtable_.setattr_ = &Generic_SetAttr;
  vtable_.call_ = &Generic_Call;
  vtable_.new_instance_ = &Generic_NewInstance;
  vtable_.init_instance_ = &Generic_InitInstance;
  vtable_.repr_ = &Generic_Repr;
  vtable_.str_ = &Generic_Str;
  vtable_.instance_size_ = &Generic_InstanceSize;
  vtable_.iterate_ = &Generic_Iterate;
}

Maybe<void> PyObjectKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  Handle<PyDict> klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // Python中object类型之上没有父类。
  // 直接调用OrderSupers会得到一个仅含有自己的mro序列。
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate, PyObjectBuiltinMethods::Install(
                                   isolate, klass_properties,
                                   type_object(isolate)));

  // 设置类名
  set_name(PyString::New(isolate, "object"));

  return JustVoid();
}

// static
void PyObjectKlass::Finalize(Isolate* isolate) {
  isolate->set_py_object_klass(Tagged<PyObjectKlass>::null());
}

// static
Maybe<uint64_t> PyObjectKlass::Generic_Hash(Isolate* isolate,
                                            Handle<PyObject> self) {
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "unhashable type: '%s'",
                      PyObject::GetTypeName(self, isolate)->buffer());
  return kNullMaybe;
}

// static
Maybe<bool> PyObjectKlass::Generic_GetAttr(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> prop_name,
                                           bool is_try,
                                           Handle<PyObject>& out_prop_val) {
  assert(IsPyString(prop_name));
  Handle<PyObject> result;
  Handle<PyObject> getattr_func;

  // 0. 在正式逻辑前，我们先给out_prop_val设置一个没找到时的默认值
  out_prop_val = Handle<PyObject>::null();

  bool handled_by_accessor = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, handled_by_accessor,
      AccessorProxy::TryGet(isolate, self, prop_name, out_prop_val));
  if (handled_by_accessor) {
    assert(!out_prop_val.is_null());
    return Maybe<bool>(true);
  }

  // 1. 如果对象存在实例字典（__dict__），
  //    尝试直接在实例字典中查找prop_name
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self, isolate);
    if (!properties.is_null()) {
      bool found = false;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                                 properties->Get(prop_name, result, isolate));
      if (found) {
        assert(!result.is_null());
        goto found;
      }
      assert(result.is_null());
    }
  }

  // 2. 沿着 MRO 序列在类字典中查找prop_name
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, prop_name, result));
  if (!result.is_null()) {
    // 1. 如果该值是一个函数（Function），通常需要将其封装为Bound Method并返回
    //   （这样调用时 self 才会自动传入）。
    // 2. 如果该值是普通数据，直接返回。
    if (IsPyFunction(result, isolate)) {
      result = isolate->factory()->NewMethodObject(result, self);
    }
    goto found;
  }

  // 3. 沿着MRO查找__getattr__(self, name)并尝试调用
  //    注意：是在类中查找 __getattr__，而不是在实例字典中查找它！！！
  RETURN_ON_EXCEPTION(isolate,
                      Runtime_LookupPropertyInInstanceTypeMro(
                          isolate, self, ST(getattr, isolate), getattr_func));
  if (!getattr_func.is_null()) {
    Handle<PyTuple> args = PyTuple::New(isolate, 1);
    args->SetInternal(0, prop_name);

    ASSIGN_RETURN_ON_EXCEPTION(isolate, result,
                               Execution::Call(isolate, getattr_func, self,
                                               args, Handle<PyDict>::null()));
    goto found;
  }

  // 4-1. 如果是虚拟机内部查询请求，直接返回null。否则抛出错误。
  // 4-2. 还没找到，抛出 AttributeError，并通过返回 null 让上层沿
  //      MaybeHandle 语义继续传播异常。
  if (is_try) {
    goto not_found;
  } else {
    goto not_found_and_throw_error;
  }

found:
  assert(out_prop_val.is_null());
  assert(!result.is_null());
  out_prop_val = result;
  return Maybe<bool>(true);

not_found:
  assert(out_prop_val.is_null());
  assert(result.is_null());
  return Maybe<bool>(false);

not_found_and_throw_error:
  Runtime_ThrowErrorf(isolate, ExceptionType::kAttributeError,
                      "'%s' object has no attribute '%s'\n",
                      PyObject::GetTypeName(self, isolate)->buffer(),
                      Handle<PyString>::cast(prop_name)->buffer());
  return kNullMaybe;
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_GetAttrForCall(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> prop_name,
    Handle<PyObject>& self_or_null) {
  assert(IsPyString(prop_name));

  self_or_null = Handle<PyObject>::null();

  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self, isolate);
    if (!properties.is_null()) {
      bool found = false;
      if (!properties->Get(prop_name, result, isolate).To(&found)) {
        return kNullMaybeHandle;
      }
      if (found) {
        assert(!result.is_null());
        return result;
      }
      assert(result.is_null());
    }
  }

  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, prop_name, result));
  if (!result.is_null()) {
    if (IsPyFunction(result, isolate) || IsNativePyFunction(result, isolate)) {
      self_or_null = self;
    }
    return result;
  }

  Handle<PyObject> attr;
  RETURN_ON_EXCEPTION(isolate,
                      Generic_GetAttr(isolate, self, prop_name, false, attr));

  assert(!attr.is_null());
  return attr;
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_SetAttr(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> property_name,
    Handle<PyObject> property_value) {
  auto properties = PyObject::GetProperties(self, isolate);

  if (!IsPyString(property_name)) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "attribute name must be string, not '%s'",
        PyObject::GetTypeName(property_name, isolate)->buffer());
    return kNullMaybeHandle;
  }

  bool handled_by_accessor = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, handled_by_accessor,
      AccessorProxy::TrySet(isolate, self, property_name, property_value));
  if (handled_by_accessor) {
    return isolate->factory()->py_none_object();
  }

  if (properties.is_null()) [[unlikely]] {
    Runtime_ThrowErrorf(isolate, ExceptionType::kAttributeError,
                        "'%s' object has no attribute '%s'",
                        PyObject::GetTypeName(self, isolate)->buffer(),
                        Handle<PyString>::cast(property_name)->buffer());
    return kNullMaybeHandle;
  }

  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(properties, property_name, property_value, isolate));

  return isolate->factory()->py_none_object();
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_Call(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> receiver,
                                                  Handle<PyObject> args,
                                                  Handle<PyObject> kwargs) {
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "'%s' object is not callable\n",
                      PyObject::GetTypeName(self, isolate)->buffer());
  return kNullMaybeHandle;
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Handle<PyObject> instance;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, instance, isolate->factory()->NewPythonObject(receiver_type));
  return instance;
}

// static
// 默认的__init__方法什么也不会做！！！
MaybeHandle<PyObject> PyObjectKlass::Generic_InitInstance(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> instance_klass =
      PyObject::ResolveObjectKlass(instance, isolate);

  int64_t arg_count =
      args.is_null() ? 0 : Handle<PyTuple>::cast(args)->length();
  int64_t kwarg_count =
      kwargs.is_null() ? 0 : Handle<PyDict>::cast(kwargs)->occupied();

  if (arg_count > 0 || kwarg_count > 0) {
    // 报错情况 1：
    // 当前对象的类型重写了 __init__ ，即 vtable 中的 init_instance_ 指针不指向
    // object_init。
    // 但代码流却显式调用了 object_init 并且传递了参数。
    // 触发场景 ：
    // 你在子类的 __init__ 中显式调用了 object.__init__() 并传递了参数。
    if (instance_klass->vtable().init_instance_ != &Generic_InitInstance)
        [[unlikely]] {
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "object.__init__() takes exactly one argument (the "
                          "instance to initialize)");
      return kNullMaybeHandle;
    }

    // 报错情况 2：
    // 子类既没重写 __init__ 也没重写 __new__
    // 触发场景：
    // 你给一个“空空如也”的类（或者只继承 object 的类）传递了参数。
    if (instance_klass->vtable().new_instance_ == &Generic_NewInstance)
        [[unlikely]] {
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.__init__() takes exactly one argument (the "
                          "instance to initialize)",
                          PyObject::GetTypeName(instance, isolate)->buffer());
      return kNullMaybeHandle;
    }
  }

  return isolate->factory()->py_none_object();
}

// static
void PyObjectKlass::Generic_Iterate(Tagged<PyObject>, ObjectVisitor*) {
  // 对象的GC扫描逻辑在各个类型的iterate虚函数和PyObject::Iterate当中实现！
}

// static
size_t PyObjectKlass::Generic_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyObject));
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_Repr(Isolate* isolate,
                                                  Handle<PyObject> self) {
  EscapableHandleScope scope;
  char buffer[128];
  std::snprintf(buffer, sizeof(buffer), "<%s object at %p>",
                PyObject::GetTypeName(self, isolate)->buffer(),
                reinterpret_cast<void*>((*self).ptr()));
  return scope.Escape(PyString::New(isolate, buffer));
}

// static
MaybeHandle<PyObject> PyObjectKlass::Generic_Str(Isolate* isolate,
                                                 Handle<PyObject> self) {
  // 默认的__str__什么也不做，直接把操作转发给对象的__repr__进行处理
  return PyObject::Repr(isolate, self);
}

}  // namespace saauso::internal
