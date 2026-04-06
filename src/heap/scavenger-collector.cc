// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/scavenger-collector.h"

#include "src/heap/heap.h"
#include "src/heap/new-space.h"
#include "src/heap/scavenge-visitor.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

void ScavengerCollector::CollectGarbage() {
  ScavenageVisitor visitor(heap_);

  // 遍历GC ROOTS，把所有的GC ROOT从eden空间拷贝到survivor空间
  heap_->IterateRoots(&visitor);

  NewPage* scan_page = heap_->new_space_->survivor_first_page();
  Address scan_ptr = scan_page->area_start();

  while (scan_page != nullptr) {
    // 如果碰到一个没有被分配过的页，那么说明BFS队列中的所有就绪对象都已经被清空。
    // 因此不用继续往下扫描了，直接退出
    if (scan_page->area_start() == scan_page->allocation_top()) {
      break;
    }

    // 如果已经将一页扫描完毕了，那么切换到下一页继续扫描
    if (scan_ptr >= scan_page->allocation_top()) {
      scan_page = scan_page->next() ? scan_page->next() : nullptr;
      scan_ptr = scan_page == nullptr ? kNullAddress : scan_page->area_start();
      continue;
    }

    // 获取当前要扫描的对象
    Tagged<PyObject> object(scan_ptr);
    size_t instance_size = PyObject::GetInstanceSize(object);

    // 扫描该对象，将该对象持有引用的子对象全部拷贝到survivor空间
    PyObject::Iterate(object, &visitor);

    // 移动scan_ptr指针，准备扫描下一个对象
    scan_ptr += instance_size;
  }

  // 交换 eden 和 survivor 空间，并重置 survivor 空间的分配状态
  heap_->new_space()->Flip();
}

}  // namespace saauso::internal
