// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

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
  return Handle<PyList>::null();
}

}  // namespace

///////////////////////////////////////////////////////////////////////

// static
Tagged<Klass> Klass::CreateRawPythonKlass() {
  auto* isolate = Isolate::Current();
  auto klass =
      isolate->heap()->Allocate<Klass>(Heap::AllocationSpace::kMetaSpace);
  // 填充虚函数表
  klass->InitializeVTable();
  // 填充字段默认值
  klass->set_klass_properties(Handle<PyDict>::null());
  klass->set_name(Handle<PyString>::null());
  klass->set_type_object(Handle<PyTypeObject>::null());
  klass->set_supers(Handle<PyList>::null());
  klass->set_mro(Handle<PyList>::null());

  return klass;
}

///////////////////////////////////////////////////////////////////////

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
  assert(klass_properties_.is_null() && "can't reset klass's properties dict");
  klass_properties_ = *klass_properties;
}

Handle<PyList> Klass::supers() {
  return handle(Tagged<PyList>::cast(supers_));
}

void Klass::set_supers(Handle<PyList> supers) {
  supers_ = *supers;
}

Handle<PyList> Klass::mro() {
  assert(!mro_.is_null());  // mro序列初始化完毕前禁止获取
  return handle(Tagged<PyList>::cast(mro_));
}

void Klass::set_mro(Handle<PyList> mro) {
  mro_ = *mro;
}

void Klass::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&name_);
  v->VisitPointer(&type_object_);
  v->VisitPointer(&klass_properties_);
  v->VisitPointer(&supers_);
  v->VisitPointer(&mro_);
}

void Klass::AddSuper(Tagged<Klass> super) {
  if (supers_.is_null()) {
    set_supers(PyList::NewInstance());
  }
  PyList::Append(supers(), super->type_object());
}

void Klass::OrderSupers() {
  // 不允许重复执行C3算法
  assert(mro_.is_null());

  HandleScope scope;
  Handle<PyList> mro_result;

  if (supers_.is_null() || supers()->IsEmpty()) {
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
  (void)PyDict::PutMaybe(klass_properties(), ST(mro), mro_result);

  mro_ = *mro_result;
}

MaybeHandle<PyObject> Klass::ConstructInstance(Handle<PyObject> args,
                                                Handle<PyObject> kwargs) {
  return vtable_.construct_instance(Tagged<Klass>(this), args, kwargs);
}

}  // namespace saauso::internal
