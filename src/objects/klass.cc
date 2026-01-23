// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

Handle<PyObject> FindAndCall(Handle<PyObject> object,
                             Handle<PyTuple> args,
                             Handle<PyObject> func_name) {
  Handle<PyObject> func = PyObject::GetAttr(object, func_name);
  if (!IsPyNone(func)) {
    return Isolate::Current()->interpreter()->CallVirtual(func, args);
  }

  std::printf("class ");
  PyObject::Print(PyObject::GetKlass(object)->name());
  std::printf(" Error : unsupport operation for class ");
  std::exit(1);

  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> FindPropertyInMro(Handle<PyObject> object,
                                   Handle<PyObject> prop_name) {
  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = PyObject::GetKlass(object)->mro();
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object =
        handle(Tagged<PyTypeObject>::cast(*mro_of_object->Get(i)));
    auto own_klass = type_object->own_klass();
    auto klass_properties = own_klass->klass_properties();
    auto result = klass_properties->Get(prop_name);
    if (!result.IsNull()) {
      return result;
    }
  }

  return Handle<PyObject>::Null();
}

Handle<PyList> C3Impl_Linear(Handle<PyTypeObject> type_object) {
  Handle<PyList> result = PyList::NewInstance();
  Handle<PyList> mro = type_object->mro();

  // 在C3Impl_Merge中我们会不断地从super的mro序列中移除元素，
  // 因此为了不影响父类的mro序列数据，我们这里需要手工（潜）拷贝
  // 一份父类的mro序列。
  for (auto i = 0; i < mro->length(); ++i) {
    PyList::Append(result, mro->Get(i));
  }

  return result;
}

Handle<PyList> C3Impl_Merge(Handle<PyList> mro_of_each_super) {
  // 递归出口：
  // 如果所有的mro列表都被清空，这意味着完整的所求mro序列已经生成，退出递归即可
  if (mro_of_each_super->IsEmpty()) {
    return PyList::NewInstance();
  }

  for (auto i = 0; i < mro_of_each_super->length(); ++i) {
    bool valid = true;

    auto mro_of_curr_super = Handle<PyList>::cast(mro_of_each_super->Get(i));
    auto head = handle(Tagged<PyTypeObject>::cast(*mro_of_curr_super->Get(0)));

    for (auto j = 0; j < mro_of_each_super->length(); ++j) {
      if (j == i) {
        continue;
      }

      Handle<PyList> mro_of_another_super =
          Handle<PyList>::cast(mro_of_each_super->Get(j));
      // 如果发现head在其他mro序列的中间（而非开头或不存在），
      // 则当前head不是所求mro序列中的下一个元素！
      if (mro_of_another_super->IndexOf(head) > 0) {
        valid = false;
        break;
      }
    }

    // mro_of_curr_super的开头不是我们想找的所求mro序列中的下一个元素，
    // 再换一个super的mro列表试试看~
    if (!valid) {
      continue;
    }

    // 否则，head就是所求mro序列中的下一个元素，将它从所有mro列表中移除，
    // 然后递归重复这个过程，直到所有mro列表被清空！
    Handle<PyList> next_mro_of_each_super = PyList::NewInstance();
    for (auto j = 0; j < mro_of_each_super->length(); ++j) {
      auto mro_of_a_super = Handle<PyList>::cast(mro_of_each_super->Get(j));
      mro_of_a_super->Remove(head);
      if (!mro_of_a_super->IsEmpty()) {
        PyList::Append(next_mro_of_each_super, mro_of_a_super);
      }
    }

    Handle<PyList> result = C3Impl_Merge(next_mro_of_each_super);
    PyList::Insert(result, 0, head);

    return result;
  }

  assert(0 && "unreachable");
  return Handle<PyList>::Null();
}

}  // namespace

///////////////////////////////////////////////////////////////////////

void Klass::InitializeVTable() {
  vtable_.add = &Klass::Virtual_Default_Add;
  vtable_.getattr = &Klass::Virtual_Default_GetAttr;
  vtable_.setattr = &Klass::Virtual_Default_SetAttr;
  vtable_.subscr = &Klass::Virtual_Default_Subscr;
  vtable_.store_subscr = &Klass::Virtual_Default_StoreSubscr;
  vtable_.next = &Klass::Virtual_Default_Next;
  vtable_.call = &Klass::Virtual_Default_Call;
  vtable_.len = &Klass::Virtual_Default_Len;
  vtable_.print = &Klass::Virtual_Default_Print;
  vtable_.repr = &Klass::Virtual_Default_Repr;
  vtable_.del_subscr = &Klass::Virtual_Default_Delete_Subscr;
  vtable_.instance_size = &Klass::Virtual_Default_InstanceSize;
}

Handle<PyString> Klass::name() {
  return Handle<PyString>(Tagged<PyString>::cast(name_));
}

void Klass::set_name(Handle<PyString> name) {
  name_ = *name;
}

Handle<PyTypeObject> Klass::type_object() {
  return Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(type_object_));
}

void Klass::set_type_object(Handle<PyTypeObject> type_object) {
  type_object_ = *type_object;
}

Handle<PyDict> Klass::klass_properties() {
  return handle(Tagged<PyDict>::cast(klass_properties_));
}

void Klass::set_klass_properties(Handle<PyDict> klass_properties) {
  assert(klass_properties_.IsNull() && "can't reset klass's properties dict");
  klass_properties_ = *klass_properties;
}

Handle<PyList> Klass::supers() {
  return handle(Tagged<PyList>::cast(supers_));
}

void Klass::set_supers(Handle<PyList> supers) {
  supers_ = *supers;
}

Handle<PyList> Klass::mro() {
  assert(!mro_.IsNull());  // mro序列初始化完毕前禁止获取
  return handle(Tagged<PyList>::cast(mro_));
}

void Klass::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&name_);
  v->VisitPointer(&type_object_);
  v->VisitPointer(&klass_properties_);
  v->VisitPointer(&supers_);
  v->VisitPointer(&mro_);
}

void Klass::AddSuper(Tagged<Klass> super) {
  if (supers_.IsNull()) {
    set_supers(PyList::NewInstance());
  }
  PyList::Append(supers(), super->type_object());
}

void Klass::OrderSupers() {
  // 不允许重复执行C3算法
  assert(mro_.IsNull());

  HandleScope scope;
  Handle<PyList> mro_result;

  if (supers_.IsNull() || supers()->IsEmpty()) {
    mro_result = PyList::NewInstance();
  } else {
    Handle<PyList> all = PyList::NewInstance(supers()->length());
    for (auto i = 0; i < supers()->length(); ++i) {
      auto super = handle(Tagged<PyTypeObject>::cast(*supers()->Get(i)));
      PyList::Append(all, C3Impl_Linear(super));
    }
    mro_result = C3Impl_Merge(all);
  }

  // 把自己添加到mro序列的开头
  PyList::Insert(mro_result, 0, type_object());
  PyDict::Put(klass_properties(), ST(mro), mro_result);

  mro_ = *mro_result;
}

///////////////////////////////////////////////////////////////////////

void Klass::Virtual_Default_Print(Handle<PyObject> self) {
  Handle<PyObject> func = PyObject::GetAttr(self, ST(str));
  if (IsPyNone(func)) {
    func = PyObject::GetAttr(self, ST(repr));
  }

  if (!IsPyNone(func)) {
    Handle<PyObject> s = Isolate::Current()->interpreter()->CallVirtual(
        func, Handle<PyTuple>::Null());
    PyObject::Print(Handle<PyString>::cast(s));
    return;
  }

  std::printf("<object at %p>", reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> Klass::Virtual_Default_Len(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyTuple>::Null(), ST(len));
}

Handle<PyObject> Klass::Virtual_Default_Repr(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyTuple>::Null(), ST(repr));
}

Handle<PyObject> Klass::Virtual_Default_Call(Handle<PyObject> self,
                                             Handle<PyObject> host,
                                             Handle<PyObject> args,
                                             Handle<PyObject> kwargs) {
  Handle<PyObject> callable = PyObject::GetAttr(self, ST(call));
  if (IsPyNone(callable)) {
    PyObject::Print(callable);
    std::printf(" is non-callable\n");
    std::exit(1);
  }

  return Isolate::Current()->interpreter()->CallVirtual(
      callable, Handle<PyTuple>::cast(args));
}

Handle<PyObject> Klass::Virtual_Default_GetAttr(Handle<PyObject> self,
                                                Handle<PyObject> prop_name) {
  assert(IsPyString(prop_name));

  // 1. 如果对象存在实例字典（__dict__），
  //    尝试直接在实例字典中查找prop_name
  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    Handle<PyDict> properties = PyObject::GetProperties(self);
    if (!properties.IsNull()) {
      result = properties->Get(prop_name);
      if (!result.IsNull()) {
        return result;
      }
    }
  }

  // 2. 沿着 MRO 序列在类字典中查找prop_name
  result = FindPropertyInMro(self, prop_name);
  if (!result.IsNull()) {
    // 1. 如果该值是一个函数（Function），通常需要将其封装为Bound Method并返回
    //   （这样调用时 self 才会自动传入）。
    // 2. 如果该值是普通数据，直接返回。
    if (IsPyFunction(result) || IsPyNativeFunction(result)) {
      result = MethodObject::NewInstance(
          handle(Tagged<PyFunction>::cast(*result)), self);
    }
    return result;
  }

  // 3. 沿着MRO查找__getattr__(self, name)并尝试调用
  //    注意：是在类中查找 __getattr__，而不是在实例字典中查找它！！！
  Handle<PyObject> getattr_func = FindPropertyInMro(self, ST(getattr));
  if (!getattr_func.IsNull()) {
    getattr_func = MethodObject::NewInstance(getattr_func, self);
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, prop_name);
    return Isolate::Current()->interpreter()->CallVirtual(getattr_func, args);
  }

  // 4. 还没找到，抛出错误并崩溃
  // AttributeError: 'A' object has no attribute 'xxx'
  std::printf("AttributeError: '");
  PyObject::Print(PyObject::GetKlass(self)->name());
  std::printf("' object has no attribute '");
  PyObject::Print(prop_name);
  std::printf("'\n");

  return Handle<PyObject>::Null();
}

void Klass::Virtual_Default_SetAttr(Handle<PyObject> self,
                                    Handle<PyObject> property_name,
                                    Handle<PyObject> property_value) {
  auto properties = PyObject::GetProperties(self);

  if (properties.IsNull()) {
    // 对齐cpython: 对于不存在__dict__的对象，直接抛错误
    std::printf("AttributeError: '");
    PyObject::Print(PyObject::GetKlass(self)->name());
    std::printf("' object has no attribute '");
    PyObject::Print(property_name);
    std::printf("'\n");
    std::exit(1);
  }

  PyDict::Put(properties, property_name, property_value);
}

Handle<PyObject> Klass::Virtual_Default_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);
  return FindAndCall(self, args, ST(getitem));
}

void Klass::Virtual_Default_StoreSubscr(Handle<PyObject> self,
                                        Handle<PyObject> subscr,
                                        Handle<PyObject> value) {
  Handle<PyTuple> args = PyTuple::NewInstance(2);
  args->SetInternal(0, subscr);
  args->SetInternal(0, value);
  FindAndCall(self, args, ST(setitem));
}

void Klass::Virtual_Default_Delete_Subscr(Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);
  FindAndCall(self, args, ST(delitem));
}

Handle<PyObject> Klass::Virtual_Default_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);
  return FindAndCall(self, args, ST(add));
}

Handle<PyObject> Klass::Virtual_Default_Next(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyTuple>::Null(), ST(getitem));
}

size_t Klass::Virtual_Default_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyObject);
}

}  // namespace saauso::internal
