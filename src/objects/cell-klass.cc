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
Tagged<CellKlass> CellKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<CellKlass> instance = isolate->cell_klass();
  if (instance.is_null()) [[unlikely]] {
    instance =
        isolate->heap()->Allocate<CellKlass>(Heap::AllocationSpace::kMetaSpace);
    isolate->set_cell_klass(instance);
  }
  return instance;
}

void CellKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void CellKlass::Initialize() {}

void CellKlass::Finalize() {
  Isolate::Current()->set_cell_klass(Tagged<CellKlass>::null());
}

// static
size_t CellKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(PyObject::GetKlass(self).ptr() ==
         Isolate::Current()->cell_klass().ptr());
  return ObjectSizeAlign(sizeof(Cell));
}

// static
void CellKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(PyObject::GetKlass(self).ptr() ==
         Isolate::Current()->cell_klass().ptr());
  Tagged<Cell> cell = Tagged<Cell>::cast(self);
  v->VisitPointer(&cell->value_);
}

}  // namespace saauso::internal
