// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

Handle<PyList> C3Impl_Linear(Isolate* isolate,
                             Handle<PyTypeObject> type_object) {
  Handle<PyList> result = PyList::New(isolate);
  Handle<PyList> mro = type_object->mro(isolate);

  // 在C3Impl_Merge中我们会不断地从super的mro序列中移除元素，
  // 因此为了不影响父类的mro序列数据，我们这里需要手工（潜）拷贝
  // 一份父类的mro序列。
  for (auto i = 0; i < mro->length(); ++i) {
    PyList::Append(result, mro->Get(i, isolate), isolate);
  }

  return result;
}

Handle<PyList> C3Impl_Merge(Isolate* isolate,
                            Handle<PyList> mro_of_each_super) {
  // 递归出口：
  // 如果所有的mro列表都被清空，这意味着完整的所求mro序列已经生成，退出递归即可
  if (mro_of_each_super->IsEmpty()) {
    return PyList::New(isolate);
  }

  for (auto i = 0; i < mro_of_each_super->length(); ++i) {
    bool valid = true;

    auto mro_of_curr_super =
        Handle<PyList>::cast(mro_of_each_super->Get(i, isolate));
    auto head =
        handle(Tagged<PyTypeObject>::cast(*mro_of_curr_super->Get(0, isolate)),
               isolate);

    for (auto j = 0; j < mro_of_each_super->length(); ++j) {
      if (j == i) {
        continue;
      }

      Handle<PyList> mro_of_another_super =
          Handle<PyList>::cast(mro_of_each_super->Get(j, isolate));
      // 如果发现head在其他mro序列的中间（而非开头或不存在），
      // 则当前head不是所求mro序列中的下一个元素！
      int64_t head_pos;
      ASSIGN_RETURN_ON_EXCEPTION_VALUE(
          isolate, head_pos, mro_of_another_super->IndexOf(head, isolate),
          Handle<PyList>::null());
      if (head_pos > 0) {
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
    Handle<PyList> next_mro_of_each_super = PyList::New(isolate);
    for (auto j = 0; j < mro_of_each_super->length(); ++j) {
      auto mro_of_a_super =
          Handle<PyList>::cast(mro_of_each_super->Get(j, isolate));
      bool removed;
      ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, removed,
                                       mro_of_a_super->Remove(head, isolate),
                                       Handle<PyList>::null());
      if (!mro_of_a_super->IsEmpty()) {
        PyList::Append(next_mro_of_each_super, mro_of_a_super, isolate);
      }
    }

    Handle<PyList> result = C3Impl_Merge(isolate, next_mro_of_each_super);
    if (result.is_null()) {
      return Handle<PyList>::null();
    }
    PyList::Insert(result, 0, head, isolate);

    return result;
  }

  assert(0 && "unreachable");
  return Handle<PyList>::null();
}

}  // namespace

///////////////////////////////////////////////////////////////////////

Maybe<void> Klass::InitializeVtable(Isolate* isolate) {
  return vtable_.Initialize(isolate, Tagged<Klass>(this));
}

Handle<PyString> Klass::name(Isolate* isolate) {
  return Handle<PyString>(Tagged<PyString>::cast(name_), isolate);
}

void Klass::set_name(Handle<PyString> name) {
  name_ = *name;
}

Handle<PyTypeObject> Klass::type_object(Isolate* isolate) {
  return Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(type_object_),
                              isolate);
}

void Klass::set_type_object(Handle<PyTypeObject> type_object) {
  type_object_ = *type_object;
}

Handle<PyDict> Klass::klass_properties(Isolate* isolate) {
  return handle(Tagged<PyDict>::cast(klass_properties_), isolate);
}

void Klass::set_klass_properties(Handle<PyDict> klass_properties) {
  assert(klass_properties_.is_null() && "can't reset klass's properties dict");
  klass_properties_ = *klass_properties;
}

Handle<PyList> Klass::supers(Isolate* isolate) {
  return handle(Tagged<PyList>::cast(supers_), isolate);
}

void Klass::set_supers(Handle<PyList> supers) {
  supers_ = *supers;
}

Handle<PyList> Klass::mro(Isolate* isolate) {
  assert(!mro_.is_null());  // mro序列初始化完毕前禁止获取
  return handle(Tagged<PyList>::cast(mro_), isolate);
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

void Klass::AddSuper(Tagged<Klass> super, Isolate* isolate) {
  if (supers_.is_null()) {
    set_supers(PyList::New(isolate));
  }
  PyList::Append(supers(isolate), super->type_object(isolate), isolate);
}

Maybe<void> Klass::OrderSupers(Isolate* isolate) {
  // 不允许重复执行C3算法
  assert(mro_.is_null());

  HandleScope scope;
  Handle<PyList> mro_result;

  if (supers_.is_null() || supers(isolate)->IsEmpty()) {
    mro_result = PyList::New(isolate);
  } else {
    Handle<PyList> all = PyList::New(isolate, supers(isolate)->length());
    for (auto i = 0; i < supers(isolate)->length(); ++i) {
      auto super =
          handle(Tagged<PyTypeObject>::cast(*supers(isolate)->Get(i, isolate)),
                 isolate);
      PyList::Append(all, C3Impl_Linear(isolate, super), isolate);
    }
    mro_result = C3Impl_Merge(isolate, all);
    assert(!mro_result.is_null());
  }

  // 把自己添加到mro序列的开头
  PyList::Insert(mro_result, 0, type_object(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(klass_properties(isolate), ST(mro, isolate),
                                  mro_result, isolate));

  mro_ = *mro_result;

  return JustVoid();
}

MaybeHandle<PyTypeObject> Klass::CreateAndBindToPyTypeObject(Isolate* isolate) {
  Handle<PyTypeObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, type_object,
                             isolate->factory()->NewPyTypeObject());
  type_object->BindWithKlass(Tagged<Klass>(this), isolate);

  return type_object;
}

MaybeHandle<PyObject> Klass::NewInstance(Isolate* isolate,
                                         Handle<PyTypeObject> receiver_type,
                                         Handle<PyObject> args,
                                         Handle<PyObject> kwargs) {
  // 任何有效的Python类型都必须实现__new__语义
  assert(vtable_.new_instance_ != nullptr);
  return vtable_.new_instance_(isolate, receiver_type, args, kwargs);
}

MaybeHandle<PyObject> Klass::InitInstance(Isolate* isolate,
                                          Handle<PyObject> instance,
                                          Handle<PyObject> args,
                                          Handle<PyObject> kwargs) {
  assert(vtable_.init_instance_);
  return vtable_.init_instance_(isolate, instance, args, kwargs);
}

}  // namespace saauso::internal
