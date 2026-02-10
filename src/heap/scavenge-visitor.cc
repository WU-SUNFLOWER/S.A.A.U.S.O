// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/scavenge-visitor.h"

#include <cstring>
#include <iostream>

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/mark-word.h"
#include "src/objects/py-object.h"


namespace saauso::internal {

namespace {

Address AllocateInSurvivorSpace(size_t size) {
  Address target_addr =
      Isolate::Current()->heap()->new_space().survivor_space().AllocateRaw(
          size);
  // survivor space中理论上一定有剩余的空间
  assert(target_addr != kNullAddress &&
         "survivor space must have enough space!!!");
  return target_addr;
}

}  // namespace

void ScavenageVisitor::VisitPointers(Tagged<PyObject>* start,
                                     Tagged<PyObject>* end) {
  for (Tagged<PyObject>* p = start; p < end; ++p) {
    Tagged<PyObject> object = *p;

    // 跳过空指针、Smi、指向meta space和不指向new space的指针
    if (object.is_null() || !IsGcAbleObject(object) ||
        !Isolate::Current()->heap()->InNewSpaceEden(object.ptr())) {
      continue;
    }

    // 移动对象，并更新槽位
    EvacuateObject(p);
  }
}

void ScavenageVisitor::VisitKlass(Tagged<Klass>* p) {
  assert(p != nullptr);
  if (p->is_null()) {
    return;
  }
  (*p)->Iterate(this);
}

void ScavenageVisitor::EvacuateObject(Tagged<PyObject>* slot_ptr) {
  Tagged<PyObject> object = *slot_ptr;

  // 如果当前slot处指针指向的对象已经被转发
  MarkWord mark_word = PyObject::GetMarkWord(object);
  if (mark_word.IsForwardingAddress()) {
    *slot_ptr = mark_word.ToForwardingAddress();
    return;
  }

  // 在Survivor Space分配空间
  size_t size = PyObject::GetInstanceSize(object);
  Address target_addr = AllocateInSurvivorSpace(size);

  // 执行拷贝
  std::memcpy(reinterpret_cast<void*>(target_addr),
              reinterpret_cast<void*>(object.ptr()), size);

  // 在原对象所处的内存位置，安放forwarding pointer
  PyObject::SetMapWordForwarded(object, Tagged<PyObject>(target_addr));

  // 更新存放PyObject*的slot
  *slot_ptr = Tagged<PyObject>(target_addr);

  // 注意：我们这里并没有递归处理object的子引用
  // 这将在Scavenage算法的主循环中完成 (无递归)
}

}  // namespace saauso::internal
