// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/cell-klass.h"

#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/cell.h"
#include "src/objects/py-object.h"
#include "src/objects/visitors.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// static
Tagged<CellKlass> CellKlass::GetInstance(Isolate* isolate) {
  Tagged<CellKlass> instance = isolate->cell_klass();
  if (instance.is_null()) [[unlikely]] {
    instance =
        isolate->heap()->Allocate<CellKlass>(Heap::AllocationSpace::kMetaSpace);
    isolate->set_cell_klass(instance);
  }
  return instance;
}

void CellKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> CellKlass::Initialize(Isolate* isolate) {
  return JustVoid();
}

void CellKlass::Finalize(Isolate* isolate) {
  isolate->set_cell_klass(Tagged<CellKlass>::null());
}

// static
size_t CellKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(Cell));
}

// static
void CellKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  Tagged<Cell> cell = Tagged<Cell>::cast(self);
  v->VisitPointer(&cell->value_);
}

}  // namespace saauso::internal
