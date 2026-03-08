// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

namespace {

// 用于在不支持的比较运算上抛出 TypeError 异常，而不是直接退出进程。
void ThrowCompareUnsupported(Handle<PyObject> self,
                             Handle<PyObject> other,
                             const char* op) {
  auto self_name = PyObject::GetKlass(self)->name();
  auto other_name = PyObject::GetKlass(other)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "'%s' not supported between instances of '%s' and '%s'\n",
                      op, self_name->buffer(), other_name->buffer());
}

}  // namespace

void Klass::InitializeVTable() {
  vtable_.add = &Klass::Virtual_Default_Add;
  vtable_.getattr = &Klass::Virtual_Default_GetAttr;
  vtable_.setattr = &Klass::Virtual_Default_SetAttr;
  vtable_.subscr = &Klass::Virtual_Default_Subscr;
  vtable_.store_subscr = &Klass::Virtual_Default_StoreSubscr;
  vtable_.greater = &Klass::Virtual_Default_Greater;
  vtable_.less = &Klass::Virtual_Default_Less;
  vtable_.equal = &Klass::Virtual_Default_Equal;
  vtable_.not_equal = &Klass::Virtual_Default_NotEqual;
  vtable_.ge = &Klass::Virtual_Default_GreaterEqual;
  vtable_.le = &Klass::Virtual_Default_LessEqual;
  vtable_.call = &Klass::Virtual_Default_Call;
  vtable_.len = &Klass::Virtual_Default_Len;
  vtable_.print = &Klass::Virtual_Default_Print;
  vtable_.repr = &Klass::Virtual_Default_Repr;
  vtable_.del_subscr = &Klass::Virtual_Default_Delete_Subscr;
  vtable_.iter = &Klass::Virtual_Default_Iter;
  vtable_.next = &Klass::Virtual_Default_Next;
  vtable_.new_instance = &Klass::Virtual_Default_NewInstance;
  vtable_.init_instance = &Klass::Virtual_Default_InitInstance;
  vtable_.hash = &Klass::Virtual_Default_Hash;
  vtable_.iterate = &Klass::Virtual_Default_Iterate;
  vtable_.instance_size = &Klass::Virtual_Default_InstanceSize;
}

Maybe<uint64_t> Klass::Virtual_Default_Hash(Handle<PyObject> self) {
  Runtime_ThrowErrorf(ExceptionType::kTypeError, "unhashable type: '%s'",
                      PyObject::GetKlass(self)->name()->buffer());
  return kNullMaybe;
}

MaybeHandle<PyObject> Klass::Virtual_Default_Print(Handle<PyObject> self) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(str), method));

  if (method.is_null()) {
    RETURN_ON_EXCEPTION(isolate, PyObject::LookupAttr(self, ST(repr), method));
  }

  if (!method.is_null()) {
    Handle<PyObject> s;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, s,
        Execution::Call(isolate, method, self, Handle<PyTuple>::null(),
                        Handle<PyDict>::null()));
    MaybeHandle<PyObject> print_result = PyObject::Print(s);
    if (print_result.IsEmpty()) {
      return kNullMaybeHandle;
    }
    return handle(isolate->py_none_object());
  }

  std::printf("<object at %p>", reinterpret_cast<void*>((*self).ptr()));
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Klass::Virtual_Default_Len(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(len))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Klass::Virtual_Default_Repr(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(repr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Klass::Virtual_Default_Call(Handle<PyObject> self,
                                                  Handle<PyObject> host,
                                                  Handle<PyObject> args,
                                                  Handle<PyObject> kwargs) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(call), callable));

  if (callable.is_null()) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "'%s' object is not callable\n",
                        PyObject::GetKlass(self)->name()->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyObject> result;
  if (!Execution::Call(isolate, callable, host, Handle<PyTuple>::cast(args),
                       Handle<PyDict>::cast(kwargs))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

Maybe<bool> Klass::Virtual_Default_GetAttr(Handle<PyObject> self,
                                           Handle<PyObject> prop_name,
                                           bool is_try,
                                           Handle<PyObject>& out_prop_val) {
  assert(IsPyString(prop_name));

  auto* isolate = Isolate::Current();
  Handle<PyObject> result;
  Handle<PyObject> getattr_func;

  // 0. 在正式逻辑前，我们先给out_prop_val设置一个没找到时的默认值
  out_prop_val = Handle<PyObject>::null();

  // 1. 如果对象存在实例字典（__dict__），
  //    尝试直接在实例字典中查找prop_name
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self);
    if (!properties.is_null()) {
      bool found = false;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                                 properties->Get(prop_name, result));
      if (found) {
        assert(!result.is_null());
        goto found;
      }
      assert(result.is_null());
    }
  }

  // 2. 沿着 MRO 序列在类字典中查找prop_name
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, prop_name, result));
  if (!result.is_null()) {
    // 1. 如果该值是一个函数（Function），通常需要将其封装为Bound Method并返回
    //   （这样调用时 self 才会自动传入）。
    // 2. 如果该值是普通数据，直接返回。
    if (IsPyFunction(result)) {
      result = MethodObject::NewInstance(
          handle(Tagged<PyFunction>::cast(*result)), self);
    }
    goto found;
  }

  // 3. 沿着MRO查找__getattr__(self, name)并尝试调用
  //    注意：是在类中查找 __getattr__，而不是在实例字典中查找它！！！
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(getattr), getattr_func));
  if (!getattr_func.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
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
  Runtime_ThrowErrorf(ExceptionType::kAttributeError,
                      "'%s' object has no attribute '%s'\n",
                      PyObject::GetKlass(self)->name()->buffer(),
                      Handle<PyString>::cast(prop_name)->buffer());
  return kNullMaybe;
}

MaybeHandle<PyObject> Klass::Virtual_Default_GetAttrForCall(
    Handle<PyObject> self,
    Handle<PyObject> prop_name,
    Handle<PyObject>& self_or_null) {
  assert(IsPyString(prop_name));

  auto* isolate = Isolate::Current();

  self_or_null = Handle<PyObject>::null();

  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self);
    if (!properties.is_null()) {
      bool found = false;
      if (!properties->Get(prop_name, result).To(&found)) {
        return kNullMaybeHandle;
      }
      if (found) {
        assert(!result.is_null());
        return result;
      }
      assert(result.is_null());
    }
  }

  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, prop_name, result));
  if (!result.is_null()) {
    if (IsPyFunction(result) || IsNativePyFunction(result)) {
      self_or_null = self;
      return result;
    }
    return result;
  }

  Handle<PyObject> attr;
  RETURN_ON_EXCEPTION(isolate,
                      Virtual_Default_GetAttr(self, prop_name, false, attr));

  assert(!attr.is_null());
  return attr;
}

MaybeHandle<PyObject> Klass::Virtual_Default_SetAttr(
    Handle<PyObject> self,
    Handle<PyObject> property_name,
    Handle<PyObject> property_value) {
  auto* isolate = Isolate::Current();
  auto properties = PyObject::GetProperties(self);

  if (properties.is_null()) {
    Runtime_ThrowErrorf(ExceptionType::kAttributeError,
                        "'%s' object has no attribute '%s'\n",
                        PyObject::GetKlass(self)->name()->buffer(),
                        Handle<PyString>::cast(property_name)->buffer());
    return kNullMaybeHandle;
  }

  if (PyDict::Put(properties, property_name, property_value).IsNothing()) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> Klass::Virtual_Default_Subscr(Handle<PyObject> self,
                                                    Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(getitem))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Klass::Virtual_Default_StoreSubscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr,
    Handle<PyObject> value) {
  auto* isolate = Isolate::Current();
  Handle<PyTuple> args = PyTuple::NewInstance(2);
  args->SetInternal(0, subscr);
  args->SetInternal(1, value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(setitem))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> Klass::Virtual_Default_Delete_Subscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr) {
  auto* isolate = Isolate::Current();
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(delitem))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> Klass::Virtual_Default_Add(Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(add))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

Maybe<bool> Klass::Virtual_Default_Greater(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(gt), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(self, other, ">");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> Klass::Virtual_Default_Less(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(lt), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(self, other, "<");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> Klass::Virtual_Default_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  if (self.is_identical_to(other)) {
    return Maybe<bool>(true);
  }

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(eq), callable));

  if (callable.is_null()) {
    return Maybe<bool>(false);
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> Klass::Virtual_Default_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(ne), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  Maybe<bool> eq = Virtual_Default_Equal(self, other);
  if (eq.IsNothing()) {
    return kNullMaybe;
  }
  return Maybe<bool>(!eq.ToChecked());
}

Maybe<bool> Klass::Virtual_Default_GreaterEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(ge), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Virtual_Default_Greater(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Default_Equal(self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> Klass::Virtual_Default_LessEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, self, ST(le), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Virtual_Default_Less(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Default_Equal(self, other));

  return Maybe<bool>(lt || eq);
}

MaybeHandle<PyObject> Klass::Virtual_Default_Next(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(next))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Klass::Virtual_Default_Iter(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(iter))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Klass::Virtual_Default_NewInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> instance;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, instance,
      isolate->factory()->NewPythonObject(klass_self->type_object()));
  return instance;
}

Maybe<void> Klass::Virtual_Default_InitInstance(Tagged<Klass> klass_self,
                                                Handle<PyObject> instance,
                                                Handle<PyObject> args,
                                                Handle<PyObject> kwargs) {
  auto* isolate = Isolate::Current();
  Handle<PyObject> init_method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, instance, ST(init), init_method));

  if (!init_method.is_null()) {
    RETURN_ON_EXCEPTION(isolate, Execution::Call(isolate, init_method, instance,
                                                 Handle<PyTuple>::cast(args),
                                                 Handle<PyDict>::cast(kwargs)));
  }

  return JustVoid();
}

void Klass::Virtual_Default_Iterate(Tagged<PyObject>, ObjectVisitor*) {
  // 对象的GC扫描逻辑在各个类型的iterate虚函数和PyObject::Iterate当中实现！
}

size_t Klass::Virtual_Default_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyObject));
}

}  // namespace saauso::internal
