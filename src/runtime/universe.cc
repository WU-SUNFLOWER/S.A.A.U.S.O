// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/universe.h"

#include "src/handles/handle_scope_implementer.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"

namespace saauso::internal {

Heap* Universe::heap_ = nullptr;
HandleScopeImplementer* Universe::handle_scope_implementer_ = nullptr;
Vector<Klass*> Universe::klass_list_;

Tagged<PyNone> Universe::py_none_object_(nullptr);
Tagged<PyBoolean> Universe::py_true_object_(nullptr);
Tagged<PyBoolean> Universe::py_false_object_(nullptr);

// static
void Universe::Genesis() {
  heap_ = new Heap();
  heap_->Setup();

  handle_scope_implementer_ = new HandleScopeImplementer();

  InitMetaArea();
}

// static
void Universe::InitMetaArea() {
  HandleScope scope;  // 为了避免污染根scope，这里我们创建一个内部的scope

  py_none_object_ = PyNone::NewInstance();
  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);

#define INIT_PY_KLASS(name) name##Klass::GetInstance()->Initialize();
  PY_TYPE_LIST(INIT_PY_KLASS)
#undef INIT_PY_KLASS
}

// static
void Universe::Destroy() {
#define FINALIZE_PY_KLASS(name) name##Klass::GetInstance()->Finalize();
  PY_TYPE_LIST(FINALIZE_PY_KLASS)
#undef FINALIZE_PY_KLASS

  delete handle_scope_implementer_;

  heap_->TearDown();
  delete heap_;
}

}  // namespace saauso::internal
