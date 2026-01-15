// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/isolate.h"

#include "src/handles/handle_scope_implementer.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
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

thread_local Isolate* Isolate::current_ = nullptr;

Isolate::Scope::Scope(Isolate* isolate) : previous_(Isolate::Current()) {
  Isolate::SetCurrent(isolate);
}

Isolate::Scope::~Scope() {
  Isolate::SetCurrent(previous_);
}

Isolate* Isolate::Create() {
  auto* isolate = new Isolate();
  isolate->Init();
  return isolate;
}

void Isolate::Dispose(Isolate* isolate) {
  if (isolate == nullptr) {
    return;
  }
  isolate->TearDown();
  if (Isolate::Current() == isolate) {
    Isolate::SetCurrent(nullptr);
  }
  delete isolate;
}

Isolate* Isolate::Current() {
  return current_;
}

void Isolate::SetCurrent(Isolate* isolate) {
  current_ = isolate;
}

Tagged<PyBoolean> Isolate::ToPyBoolean(bool condition) {
  Isolate* isolate = Isolate::Current();
  return condition ? isolate->py_true_object_ : isolate->py_false_object_;
}

void Isolate::Init() {
  heap_ = new Heap(this);
  heap_->Setup();

  handle_scope_implementer_ = new HandleScopeImplementer();

  Scope isolate_scope(this);
  HandleScope scope;
  InitMetaArea();
  interpreter_ = new Interpreter();
}

void Isolate::InitMetaArea() {
#define PREINIT_PY_KLASS(name)               \
  do {                                       \
    auto klass = name##Klass::GetInstance(); \
    klass->InitializeVTable();               \
    klass->PreInitialize();                  \
  } while (false);
  PY_TYPE_LIST(PREINIT_PY_KLASS)
  PREINIT_PY_KLASS(PyObject)
  PREINIT_PY_KLASS(NativeFunction)
#undef PREINIT_PY_KLASS

  string_table_ = new StringTable();

  py_none_object_ = PyNone::NewInstance();
  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);

#define INIT_PY_KLASS(name) name##Klass::GetInstance()->Initialize();
  INIT_PY_KLASS(PyObject)
  PY_TYPE_LIST(INIT_PY_KLASS)
  INIT_PY_KLASS(NativeFunction)
#undef INIT_PY_KLASS
}

void Isolate::TearDown() {
  Scope isolate_scope(this);

#define FINALIZE_PY_KLASS(name) name##Klass::GetInstance()->Finalize();
  PY_TYPE_LIST(FINALIZE_PY_KLASS)
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
