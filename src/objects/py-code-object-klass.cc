// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object-klass.h"

#include <cstdint>
#include <cstdio>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

// static
Tagged<PyCodeObjectKlass> PyCodeObjectKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyCodeObjectKlass> instance = isolate->py_code_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyCodeObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_code_object_klass(instance);
  }
  return instance;
}

void PyCodeObjectKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyCodeObjectKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  Handle<PyTypeObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, type_object,
                             isolate->factory()->NewPyTypeObject());
  type_object->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 设置类名
  set_name(PyString::NewInstance("code"));

  return JustVoid();
}

MaybeHandle<PyObject> PyCodeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto code = Handle<PyCodeObject>::cast(self);
  Tagged<PyObject> file_name_obj = code->file_name_;
  if (file_name_obj.is_null() || !IsPyString(file_name_obj)) {
    std::printf("<code object greet at 0x%p, file <unknown>, line %d>",
                reinterpret_cast<void*>((*code).ptr()), code->line_no_);
    return handle(Isolate::Current()->py_none_object());
  }
  auto file_name = handle(Tagged<PyString>::cast(file_name_obj));
  std::printf("<code object greet at 0x%p, file \"%s\", line %d>",
              reinterpret_cast<void*>((*code).ptr()), file_name->buffer(),
              code->line_no_);
  return handle(Isolate::Current()->py_none_object());
}

void PyCodeObjectKlass::Finalize(Isolate* isolate) {
  isolate->set_py_code_object_klass(Tagged<PyCodeObjectKlass>::null());
}

/////////////////////////////////////////////////////////////////////////

size_t PyCodeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyCodeObject));
}

void PyCodeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  auto code = Tagged<PyCodeObject>::cast(self);
  v->VisitPointer(&code->bytecodes_);
  v->VisitPointer(&code->names_);
  v->VisitPointer(&code->consts_);
  v->VisitPointer(&code->localsplusnames_);
  v->VisitPointer(&code->localspluskinds_);
  v->VisitPointer(&code->var_names_);
  v->VisitPointer(&code->free_vars_);
  v->VisitPointer(&code->cell_vars_);
  v->VisitPointer(&code->file_name_);
  v->VisitPointer(&code->co_name_);
  v->VisitPointer(&code->qual_name_);
  v->VisitPointer(&code->line_table_);
  v->VisitPointer(&code->exception_table_);
  v->VisitPointer(&code->no_table_);
}

}  // namespace saauso::internal
