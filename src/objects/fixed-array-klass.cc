// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/fixed-array-klass.h"

#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

Tagged<FixedArrayKlass> FixedArrayKlass::instance_(nullptr);

// static
Tagged<FixedArrayKlass> FixedArrayKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<FixedArrayKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void FixedArrayKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

// static
void FixedArrayKlass::Finalize() {
  instance_ = Tagged<FixedArrayKlass>::Null();
}

// static
size_t FixedArrayKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(IsFixedArray(self));
  return FixedArray::ComputeObjectSize(
      Tagged<FixedArray>::cast(self)->capacity());
}

// static
void FixedArrayKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsFixedArray(self));

  auto fixed_array = Tagged<FixedArray>::cast(self);
  // 扫描fixed array当中保存的数据
  v->VisitPointers(fixed_array->data(),
                   fixed_array->data() + fixed_array->capacity());
}

}  // namespace saauso::internal
