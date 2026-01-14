// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object-klass.h"

#include <cstdint>
#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

Tagged<PyCodeObjectKlass> PyCodeObjectKlass::instance_(nullptr);

// static
Tagged<PyCodeObjectKlass> PyCodeObjectKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyCodeObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyCodeObjectKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置类名
  set_name(PyString::NewInstance("code"));
}

// static
void PyCodeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto code = Handle<PyCodeObject>::Cast(self);
  auto file_name = Handle<PyString>(Tagged<PyString>::Cast(code->file_name_));
  std::printf("<code object greet at 0x%p, file \"%.*s\", line %d>",
              reinterpret_cast<void*>((*code).ptr()),
              static_cast<int>(file_name->length()), file_name->buffer(),
              code->line_no_);
}

void PyCodeObjectKlass::Finalize() {
  instance_ = Tagged<PyCodeObjectKlass>::Null();
}

/////////////////////////////////////////////////////////////////////////

size_t PyCodeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyCodeObject));
}

void PyCodeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  auto code = Tagged<PyCodeObject>::Cast(self);
  v->VisitPointer(&code->bytecodes_);
  v->VisitPointer(&code->names_);
  v->VisitPointer(&code->consts_);
  v->VisitPointer(&code->var_names_);
  v->VisitPointer(&code->free_vars_);
  v->VisitPointer(&code->cell_vars_);
  v->VisitPointer(&code->file_name_);
  v->VisitPointer(&code->co_name_);
  v->VisitPointer(&code->no_table_);
}

}  // namespace saauso::internal
