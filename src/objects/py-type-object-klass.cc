// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object-klass.h"

#include "include/saauso-internal.h"
#include "include/saauso-maybe.h"
#include "src/builtins/builtins-py-type-object-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/templates.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-type-object.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

// static
Tagged<PyTypeObjectKlass> PyTypeObjectKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyTypeObjectKlass> instance = isolate->py_type_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTypeObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_type_object_klass(instance);
  }
  return instance;
}

void PyTypeObjectKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到isolate
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.getattr_ = &Virtual_GetAttr;
  vtable_.setattr_ = &Virtual_SetAttr;
  vtable_.hash_ = &Virtual_Hash;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.call_ = &Virtual_Call;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyTypeObjectKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate, PyTypeObjectBuiltinMethods::Install(
                                   isolate, klass_properties, type_object()));

  // 设置类名
  set_name(PyString::NewInstance("type"));

  return JustVoid();
}

void PyTypeObjectKlass::Finalize(Isolate* isolate) {
  isolate->set_py_type_object_klass(Tagged<PyTypeObjectKlass>::null());
}

///////////////////////////////////////////////////////////////////////////

Maybe<bool> PyTypeObjectKlass::Virtual_GetAttr(Handle<PyObject> self,
                                               Handle<PyObject> prop_name,
                                               bool is_try,
                                               Handle<PyObject>& out_prop_val) {
  assert(IsPyString(prop_name));

  auto* isolate = Isolate::Current();
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();

  out_prop_val = Handle<PyObject>::null();

  Handle<PyObject> result;
  if (is_try) {
    // 沿着当前type object的mro序列进行查找（Lookup语义）
    RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInKlassMro(
                                     isolate, own_klass, prop_name, result));
  } else {
    // 对外语义：找不到属性时直接抛出 AttributeError（Get语义）
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Runtime_GetPropertyInKlassMro(isolate, own_klass, prop_name));
  }

  if (!result.is_null()) {
    out_prop_val = result;
    return Maybe<bool>(true);
  }
  return Maybe<bool>(false);
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_Repr(Isolate* isolate,
                                                      Handle<PyObject> self) {
  Handle<PyString> repr;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, repr,
      Runtime_NewTypeObjectRepr(isolate, Handle<PyTypeObject>::cast(self)));
  return repr;
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_Str(Isolate* isolate,
                                                     Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_SetAttr(
    Handle<PyObject> self,
    Handle<PyObject> prop_name,
    Handle<PyObject> prop_value) {
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();
  if (PyDict::Put(own_klass->klass_properties(), prop_name, prop_value)
          .IsNothing()) {
    return kNullMaybeHandle;
  }
  return handle(Isolate::Current()->py_none_object());
}

Maybe<uint64_t> PyTypeObjectKlass::Virtual_Hash(Handle<PyObject> self) {
  return Maybe<uint64_t>(static_cast<uint64_t>((*self).ptr()));
}

Maybe<bool> PyTypeObjectKlass::Virtual_Equal(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  if (!IsPyTypeObject(other)) {
    return Maybe<bool>(false);
  }
  return Maybe<bool>(self.is_identical_to(other));
}

Maybe<bool> PyTypeObjectKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  bool is_equal;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), is_equal,
                             Virtual_Equal(self, other));

  return Maybe<bool>(!is_equal);
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(receiver_type->own_klass() == PyTypeObjectKlass::GetInstance());
  return Runtime_NewType(isolate, args, kwargs);
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_Call(Isolate* isolate,
                                                      Handle<PyObject> self,
                                                      Handle<PyObject> receiver,
                                                      Handle<PyObject> args,
                                                      Handle<PyObject> kwargs) {
  auto type_object = Handle<PyTypeObject>::cast(self);
  return Runtime_NewObject(isolate, type_object, args, kwargs);
}

// static
size_t PyTypeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyTypeObject));
}

// static
void PyTypeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  // 虽然type object中有一个klass指针，
  // 但是我们约定GC阶段对klass的扫描统一走Heap::IterateRoots。
  // 因此这里不做任何事情！
}

}  // namespace saauso::internal
