// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object-klass.h"

#include "src/builtins/builtins-py-type-object-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"

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

void PyTypeObjectKlass::PreInitialize() {
  // 将自己注册到isolate
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.getattr = &Virtual_GetAttr;
  vtable_.setattr = &Virtual_SetAttr;
  vtable_.hash = &Virtual_Hash;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.construct_instance = &Virtual_ConstructInstance;
  vtable_.call = &Virtual_Call;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyTypeObjectKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  PyTypeObjectBuiltinMethods::Install(klass_properties);

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("type"));
}

void PyTypeObjectKlass::Finalize() {
  Isolate::Current()->set_py_type_object_klass(
      Tagged<PyTypeObjectKlass>::null());
}

///////////////////////////////////////////////////////////////////////////

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto type_object = Handle<PyTypeObject>::cast(self);
  auto type_name = type_object->own_klass()->name();
  std::printf("<class '%s'>", type_name->buffer());
  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> PyTypeObjectKlass::Virtual_GetAttr(Handle<PyObject> self,
                                                    Handle<PyObject> prop_name,
                                                    bool is_try) {
  assert(IsPyString(prop_name));
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();

  // 沿着当前type object的mro序列进行查找
  Handle<PyObject> result =
      Runtime_FindPropertyInKlassMro(own_klass, prop_name);
  if (!result.is_null()) {
    return result;
  }
  if (is_try) {
    return Handle<PyObject>::null();
  }
  Runtime_ThrowErrorf(ExceptionType::kAttributeError,
                      "'%s' object has no attribute '%s'\n",
                      PyObject::GetKlass(self)->name()->buffer(),
                      Handle<PyString>::cast(prop_name)->buffer());
  return Handle<PyObject>::null();
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_SetAttr(
    Handle<PyObject> self,
    Handle<PyObject> prop_name,
    Handle<PyObject> prop_value) {
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();
  PyDict::Put(own_klass->klass_properties(), prop_name, prop_value);
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

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyTypeObjectKlass::GetInstance());
  return Runtime_NewType(args, kwargs);
}

MaybeHandle<PyObject> PyTypeObjectKlass::Virtual_Call(Handle<PyObject> self,
                                                      Handle<PyObject> host,
                                                      Handle<PyObject> args,
                                                      Handle<PyObject> kwargs) {
  (void)host;
  auto type_object = Handle<PyTypeObject>::cast(self);
  auto own_klass = type_object->own_klass();
  return own_klass->ConstructInstance(args, kwargs);
}

// static
size_t PyTypeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyTypeObject);
}

// static
void PyTypeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  // 虽然type object中有一个klass指针，
  // 但是我们约定GC阶段对klass的扫描统一走Heap::IterateRoots。
  // 因此这里不做任何事情！
}

}  // namespace saauso::internal
