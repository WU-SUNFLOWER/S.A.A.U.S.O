// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

void PrintCompareUnsupported(Handle<PyObject> self,
                             Handle<PyObject> other,
                             const char* op) {
  auto self_name = PyObject::GetKlass(self)->name();
  auto other_name = PyObject::GetKlass(other)->name();
  std::fprintf(
      stderr,
      "TypeError: '%s' not supported between instances of '%s' and '%s'\n", op,
      self_name->buffer(), other_name->buffer());
  std::exit(1);
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
  vtable_.construct_instance = &Klass::Virtual_Default_ConstructInstance;
  vtable_.iterate = &Klass::Virtual_Default_Iterate;
  vtable_.instance_size = &Klass::Virtual_Default_InstanceSize;
}

void Klass::Virtual_Default_Print(Handle<PyObject> self) {
  // 先尝试查找对象的__str__方法
  Handle<PyObject> method =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(str));
  if (method.is_null()) {
    // 没有找到，再尝试查找__repr__方法
    method = PyObject::GetAttr(self, ST(repr), true);
  }

  if (!method.is_null()) {
    auto* isolate = Isolate::Current();
    Handle<PyObject> s;

    ASSIGN_RETURN_ON_EXCEPTION_VOID(
        isolate, s,
        Execution::Call(isolate, method, self, Handle<PyTuple>::null(),
                        Handle<PyDict>::null()));

    PyObject::Print(s);
    return;
  }

  // 如果重载方法都没找到，那么最后打印默认输初作为兜底
  std::printf("<object at %p>", reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> Klass::Virtual_Default_Len(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(len))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

Handle<PyObject> Klass::Virtual_Default_Repr(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(repr))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

Handle<PyObject> Klass::Virtual_Default_Call(Handle<PyObject> self,
                                             Handle<PyObject> host,
                                             Handle<PyObject> args,
                                             Handle<PyObject> kwargs) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(call));
  if (callable.is_null()) {
    std::fprintf(stderr, "TypeError: '%s' object is not callable\n",
                 PyObject::GetKlass(self)->name()->buffer());
    std::exit(1);
  }

  Handle<PyObject> result;
  if (!Execution::Call(Isolate::Current(), callable, host,
                       Handle<PyTuple>::cast(args),
                       Handle<PyDict>::cast(kwargs))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

Handle<PyObject> Klass::Virtual_Default_GetAttr(Handle<PyObject> self,
                                                Handle<PyObject> prop_name,
                                                bool is_try) {
  assert(IsPyString(prop_name));

  // 1. 如果对象存在实例字典（__dict__），
  //    尝试直接在实例字典中查找prop_name
  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self);
    if (!properties.is_null()) {
      result = properties->Get(prop_name);
      if (!result.is_null()) {
        return result;
      }
    }
  }

  // 2. 沿着 MRO 序列在类字典中查找prop_name
  result = Runtime_FindPropertyInInstanceTypeMro(self, prop_name);
  if (!result.is_null()) {
    // 1. 如果该值是一个函数（Function），通常需要将其封装为Bound Method并返回
    //   （这样调用时 self 才会自动传入）。
    // 2. 如果该值是普通数据，直接返回。
    if (IsPyFunction(result)) {
      result = MethodObject::NewInstance(
          handle(Tagged<PyFunction>::cast(*result)), self);
    }
    return result;
  }

  // 3. 沿着MRO查找__getattr__(self, name)并尝试调用
  //    注意：是在类中查找 __getattr__，而不是在实例字典中查找它！！！
  Handle<PyObject> getattr_func =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(getattr));
  if (!getattr_func.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, prop_name);

    Handle<PyObject> getattr_result;
    if (!Execution::Call(Isolate::Current(), getattr_func, self, args,
                         Handle<PyDict>::null())
             .ToHandle(&getattr_result)) {
      return Handle<PyObject>::null();
    }
    return getattr_result;
  }

  // 4-1 如果是虚拟机内部查询请求，直接返回null
  if (is_try) {
    return Handle<PyObject>::null();
  }

  // 4-2. 还没找到，抛出错误并崩溃
  // AttributeError: 'A' object has no attribute 'xxx'
  std::fprintf(stderr, "AttributeError: '%s' object has no attribute '%s'\n",
               PyObject::GetKlass(self)->name()->buffer(),
               Handle<PyString>::cast(prop_name)->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

Handle<PyObject> Klass::Virtual_Default_GetAttrForCall(
    Handle<PyObject> self,
    Handle<PyObject> prop_name,
    Handle<PyObject>& self_or_null) {
  assert(IsPyString(prop_name));

  self_or_null = Handle<PyObject>::null();

  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self);
    if (!properties.is_null()) {
      result = properties->Get(prop_name);
      if (!result.is_null()) {
        return result;
      }
    }
  }

  result = Runtime_FindPropertyInInstanceTypeMro(self, prop_name);
  if (!result.is_null()) {
    if (IsPyFunction(result) || IsPyNativeFunction(result)) {
      self_or_null = self;
      return result;
    }
    return result;
  }

  return Virtual_Default_GetAttr(self, prop_name, false);
}

void Klass::Virtual_Default_SetAttr(Handle<PyObject> self,
                                    Handle<PyObject> property_name,
                                    Handle<PyObject> property_value) {
  auto properties = PyObject::GetProperties(self);

  if (properties.is_null()) {
    // 对齐cpython: 对于不存在__dict__的对象，直接抛错误
    std::fprintf(stderr, "AttributeError: '%s' object has no attribute '%s'\n",
                 PyObject::GetKlass(self)->name()->buffer(),
                 Handle<PyString>::cast(property_name)->buffer());
    std::exit(1);
  }

  PyDict::Put(properties, property_name, property_value);
}

Handle<PyObject> Klass::Virtual_Default_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(getitem))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

void Klass::Virtual_Default_StoreSubscr(Handle<PyObject> self,
                                        Handle<PyObject> subscr,
                                        Handle<PyObject> value) {
  Handle<PyTuple> args = PyTuple::NewInstance(2);
  args->SetInternal(0, subscr);
  args->SetInternal(1, value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(setitem))
           .ToHandle(&result)) {
    return;
  }
}

void Klass::Virtual_Default_Delete_Subscr(Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(delitem))
           .ToHandle(&result)) {
    return;
  }
}

Handle<PyObject> Klass::Virtual_Default_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(add))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

bool Klass::Virtual_Default_Greater(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(gt));
  if (callable.is_null()) {
    PrintCompareUnsupported(self, other, ">");
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  auto* isolate = Isolate::Current();
  Handle<PyObject> result;

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
      false);

  return Runtime_PyObjectIsTrue(result);
}

bool Klass::Virtual_Default_Less(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(lt));
  if (callable.is_null()) {
    PrintCompareUnsupported(self, other, "<");
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  auto* isolate = Isolate::Current();
  Handle<PyObject> result;

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
      false);

  return Runtime_PyObjectIsTrue(result);
}

bool Klass::Virtual_Default_Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  if (self.is_identical_to(other)) {
    return true;
  }

  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(eq));
  if (callable.is_null()) {
    return false;
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  auto* isolate = Isolate::Current();
  Handle<PyObject> result;

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
      false);

  return Runtime_PyObjectIsTrue(result);
}

bool Klass::Virtual_Default_NotEqual(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(ne));
  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    auto* isolate = Isolate::Current();
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
        false);

    return Runtime_PyObjectIsTrue(result);
  }

  return !Virtual_Default_Equal(self, other);
}

bool Klass::Virtual_Default_GreaterEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(ge));
  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    auto* isolate = Isolate::Current();
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
        false);

    return Runtime_PyObjectIsTrue(result);
  }

  return Virtual_Default_Greater(self, other) ||
         Virtual_Default_Equal(self, other);
}

bool Klass::Virtual_Default_LessEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  Handle<PyObject> callable =
      Runtime_FindPropertyInInstanceTypeMro(self, ST(le));
  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    auto* isolate = Isolate::Current();
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()),
        false);

    return Runtime_PyObjectIsTrue(result);
  }

  return Virtual_Default_Less(self, other) ||
         Virtual_Default_Equal(self, other);
}

Handle<PyObject> Klass::Virtual_Default_Next(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(next))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

Handle<PyObject> Klass::Virtual_Default_Iter(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(iter))
           .ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

// 默认的构造新对象执行逻辑
Handle<PyObject> Klass::Virtual_Default_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  auto instance = PyObject::AllocateRawPythonObject();
  auto type_object = klass_self->type_object();

  PyObject::SetKlass(instance, type_object->own_klass());
  PyDict::Put(PyObject::GetProperties(instance), ST(class), type_object);

  // 如果用户自定义类有__init__方法，那么调用之
  auto init_method = Runtime_FindPropertyInInstanceTypeMro(instance, ST(init));
  if (!init_method.is_null()) {
    if (Execution::Call(Isolate::Current(), init_method, instance,
                        Handle<PyTuple>::cast(args),
                        Handle<PyDict>::cast(kwargs))
            .IsEmpty()) {
      return Handle<PyObject>::null();
    }
  }

  return instance;
}

void Klass::Virtual_Default_Iterate(Tagged<PyObject>, ObjectVisitor*) {
  // 对象的GC扫描逻辑在各个类型的iterate虚函数和PyObject::Iterate当中实现！
}

size_t Klass::Virtual_Default_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyObject);
}

}  // namespace saauso::internal
