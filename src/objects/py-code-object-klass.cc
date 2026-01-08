// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object-klass.h"

#include <cstdint>
#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"
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
  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;

  // 设置类名
  set_name(PyString::NewInstance("code"));
}

// static
void PyCodeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto code = Handle<PyCodeObject>::Cast(self);
  std::printf("<code object greet at 0x%p, file \"%.*s\", line %d>",
              reinterpret_cast<void*>((*code).ptr()),
              static_cast<int>(code->file_name_->length()),
              code->file_name_->buffer(), code->line_no_);
}

}  // namespace saauso::internal
