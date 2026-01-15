// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/universe.h"

#include "src/handles/handle_scope_implementer.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-type-object-klass.h"
#include "src/runtime/interpreter.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Heap* Universe::heap_ = nullptr;
HandleScopeImplementer* Universe::handle_scope_implementer_ = nullptr;
Vector<Klass*> Universe::klass_list_;
Interpreter* Universe::interpreter_ = nullptr;
StringTable* Universe::string_table_ = nullptr;

Tagged<PyNone> Universe::py_none_object_(nullptr);
Tagged<PyBoolean> Universe::py_true_object_(nullptr);
Tagged<PyBoolean> Universe::py_false_object_(nullptr);

// static
void Universe::Genesis() {
  heap_ = new Heap();
  heap_->Setup();

  handle_scope_implementer_ = new HandleScopeImplementer();

  HandleScope scope;
  InitMetaArea();
  interpreter_ = new Interpreter();
}

// static
void Universe::InitMetaArea() {
#define PREINIT_PY_KLASS(name)               \
  do {                                       \
    auto klass = name##Klass::GetInstance(); \
    klass->InitializeVTable();               \
    klass->PreInitialize();                  \
  } while (false);
  PY_TYPE_LIST(PREINIT_PY_KLASS)
  // 特化klass初始化
  PREINIT_PY_KLASS(PyObject)
  PREINIT_PY_KLASS(NativeFunction)
#undef PREINIT_PY_KLASS

  string_table_ = new StringTable();

  py_none_object_ = PyNone::NewInstance();
  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);

#define INIT_PY_KLASS(name) name##Klass::GetInstance()->Initialize();
  // PyObjectKlass是其他klass计算mro的基础，必须首先初始化！
  INIT_PY_KLASS(PyObject)
  PY_TYPE_LIST(INIT_PY_KLASS)
  // 特化klass初始化
  INIT_PY_KLASS(NativeFunction)
#undef INIT_PY_KLASS
}

// static
void Universe::Destroy() {
#define FINALIZE_PY_KLASS(name) name##Klass::GetInstance()->Finalize();
  PY_TYPE_LIST(FINALIZE_PY_KLASS)
  // 特化klass反初始化
  FINALIZE_PY_KLASS(PyObject)
  FINALIZE_PY_KLASS(NativeFunction)
#undef FINALIZE_PY_KLASS

  delete string_table_;
  string_table_ = nullptr;

  delete interpreter_;
  interpreter_ = nullptr;

  delete handle_scope_implementer_;
  handle_scope_implementer_ = nullptr;

  klass_list_.Resize(0);

  py_none_object_ = Tagged<PyNone>::Null();
  py_true_object_ = Tagged<PyBoolean>::Null();
  py_false_object_ = Tagged<PyBoolean>::Null();

  heap_->TearDown();
  delete heap_;
  heap_ = nullptr;
}

}  // namespace saauso::internal
