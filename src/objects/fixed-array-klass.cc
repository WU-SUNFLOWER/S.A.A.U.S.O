// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/fixed-array-klass.h"

#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

// static
Tagged<FixedArrayKlass> FixedArrayKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<FixedArrayKlass> instance = isolate->fixed_array_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<FixedArrayKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_fixed_array_klass(instance);
  }
  return instance;
}

void FixedArrayKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> FixedArrayKlass::Initialize(Isolate* isolate) {
  return JustVoid();
}

// static
void FixedArrayKlass::Finalize() {
  Isolate::Current()->set_fixed_array_klass(Tagged<FixedArrayKlass>::null());
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
