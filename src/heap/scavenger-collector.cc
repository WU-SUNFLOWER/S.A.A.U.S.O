// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/scavenger-collector.h"

#include "src/heap/heap.h"
#include "src/heap/scavenge-visitor.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

namespace {

NewPage* NextSurvivorPage(NewPage* page) {
  if (page == nullptr) {
    return nullptr;
  }
  NewPage* next = page->next();
  if (next == nullptr || !next->HasFlag(BasePage::Flag::kSurvivorPage)) {
    return nullptr;
  }
  return next;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////

void ScavengerCollector::CollectGarbage() {
  ScavenageVisitor visitor(heap_);

  // 遍历GC ROOTS，把所有的GC ROOT从eden空间拷贝到survivor空间
  heap_->IterateRoots(&visitor);

  NewPage* scan_page = heap_->new_space_.survivor_first_page();
  Address scan_ptr =
      scan_page == nullptr ? kNullAddress : scan_page->area_start();

  while (scan_page != nullptr) {
    if (scan_ptr >= scan_page->allocation_top()) {
      scan_page = NextSurvivorPage(scan_page);
      scan_ptr = scan_page == nullptr ? kNullAddress : scan_page->area_start();
      continue;
    }

    Tagged<PyObject> object(scan_ptr);
    size_t instance_size = PyObject::GetInstanceSize(object);

    // 扫描该对象，将该对象持有引用的子对象全部拷贝到survivor空间
    PyObject::Iterate(object, &visitor);

    // 移动scan_ptr指针，准备扫描下一个对象
    scan_ptr += instance_size;
  }

  // 交换空间
  heap_->new_space().Flip();
}

}  // namespace saauso::internal
