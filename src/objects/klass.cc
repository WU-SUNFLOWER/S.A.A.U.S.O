// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/interpreter.h"
#include "src/runtime/string-table.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

namespace {

Handle<PyObject> FindAndCall(Handle<PyObject> object,
                             Handle<PyList> args,
                             Handle<PyObject> func_name) {
  Handle<PyObject> func = PyObject::GetAttr(object, func_name);
  if (!IsPyNone(func)) {
    return Universe::interpreter_->CallVirtual(func, args);
  }

  std::printf("class ");
  PyObject::Print(PyObject::GetKlass(object)->name());
  std::printf(" Error : unsupport operation for class ");
  std::exit(1);

  return handle(Universe::py_none_object_);
}

Handle<PyObject> FindPropertyInMro(Handle<PyObject> object,
                                   Handle<PyObject> prop_name) {
  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = PyObject::GetKlass(object)->mro();
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object = Handle<PyTypeObject>::cast(mro_of_object->Get(0));
    auto result = type_object->own_klass()->klass_properties()->Get(prop_name);
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
    PyList::Append(result, mro->Get(0));
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
    auto head = Handle<PyTypeObject>::cast(mro_of_curr_super->Get(0));

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

VirtualTable::VirtualTable()
    : add(&Klass::Virtual_Default_Add),
      getattr(&Klass::Virtual_Default_GetAttr),
      setattr(&Klass::Virtual_Default_SetAttr),
      subscr(&Klass::Virtual_Default_Subscr),
      store_subscr(&Klass::Virtual_Default_StoreSubscr),
      next(&Klass::Virtual_Default_Next),
      call(&Klass::Virtual_Default_Call),
      len(&Klass::Virtual_Default_Len),
      print(&Klass::Virtual_Default_Print),
      repr(&Klass::Virtual_Default_Repr),
      del_subscr(&Klass::Virtual_Default_Delete_Subscr),
      instance_size(&Klass::Virtual_Default_InstanceSize) {}

///////////////////////////////////////////////////////////////////////

Handle<PyString> Klass::name() {
  return Handle<PyString>(Tagged<PyString>::cast(name_));
}

void Klass::set_name(Handle<PyString> name) {
  name_ = *name;
}

Handle<PyTypeObject> Klass::type_object() {
  return Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(name_));
}

void Klass::set_type_object(Handle<PyTypeObject> type_object) {
  type_object_ = *type_object;
}

Handle<PyDict> Klass::klass_properties() {
  return handle(Tagged<PyDict>::cast(klass_properties_));
}

void Klass::set_klass_properties(Handle<PyDict> klass_properties) {
  type_object_ = *klass_properties;
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
      auto super = Handle<PyTypeObject>::cast(supers()->Get(i));
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
    Handle<PyObject> s =
        Universe::interpreter_->CallVirtual(func, Handle<PyList>::Null());
    PyObject::Print(Handle<PyString>::cast(s));
    return;
  }

  std::printf("<object at %p>", reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> Klass::Virtual_Default_Len(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(len));
}

Handle<PyObject> Klass::Virtual_Default_Repr(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(repr));
}

Handle<PyObject> Klass::Virtual_Default_Call(Handle<PyObject> self,
                                             Handle<PyObject> args,
                                             Handle<PyObject> kwargs) {
  Handle<PyObject> callable = PyObject::GetAttr(self, ST(call));
  if (IsPyNone(callable)) {
    PyObject::Print(callable);
    std::printf(" is non-callable\n");
    std::exit(1);
  }

  return Universe::interpreter_->CallVirtual(callable,
                                             Handle<PyList>::cast(args));
}

Handle<PyObject> Klass::Virtual_Default_GetAttr(Handle<PyObject> self,
                                                Handle<PyObject> prop_name) {
  assert(IsPyString(prop_name));

  Handle<PyObject> getattr_func = FindPropertyInMro(self, prop_name);

  // 如果父类或者祖先类当中定义了__getattr__方法，就先调用这个方法
  if (!getattr_func.IsNull()) {
    getattr_func =
        MethodObject::NewInstance(Handle<PyFunction>::cast(getattr_func), self);
    Handle<PyList> args = PyList::NewInstance(1);
    PyList::Append(args, prop_name);
    return Universe::interpreter_->CallVirtual(getattr_func, args);
  }

  // 如果没有定义 __getattr__方法，就去对象字典里找
  Handle<PyObject> result;
  if (IsHeapObject(self)) {
    result = PyObject::GetProperties(self)->Get(prop_name);
    if (!result.IsNull()) {
      return result;
    }
  }

  // 如果对象字典里也没有，就去自己的类里找
  result = FindPropertyInMro(self, prop_name);
  if (!result.IsNull()) {
    if (IsPyFunction(result) || IsPyNativeFunction(result)) {
      result =
          MethodObject::NewInstance(Handle<PyFunction>::cast(result), self);
    }
    return result;
  }

  // 还没找到，抛出错误并崩溃
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
  PyDict::Put(properties, property_name, property_value);
}

Handle<PyObject> Klass::Virtual_Default_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  return FindAndCall(self, args, ST(getitem));
}

void Klass::Virtual_Default_StoreSubscr(Handle<PyObject> self,
                                        Handle<PyObject> subscr,
                                        Handle<PyObject> value) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  PyList::Append(args, value);
  FindAndCall(self, args, ST(setitem));
}

void Klass::Virtual_Default_Delete_Subscr(Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  FindAndCall(self, args, ST(delitem));
}

Handle<PyObject> Klass::Virtual_Default_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, other);
  return FindAndCall(self, args, ST(add));
}

Handle<PyObject> Klass::Virtual_Default_Next(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(getitem));
}

size_t Klass::Virtual_Default_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyObject);
}

}  // namespace saauso::internal
