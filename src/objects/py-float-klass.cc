// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "heap/heap.h"
#include "objects/py-float-klass.h"
#include "objects/py-string.h"
#include "runtime/universe.h"


namespace saauso::internal {

PyFloatKlass* PyFloatKlass::instance_ = nullptr;

PyFloatKlass* PyFloatKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyFloatKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyFloatKlass::Initialize() {
  // 初始化虚函数表
  vtable_.add = &Virtual_Add;
  vtable_.sub = &Virtual_Sub;
  vtable_.mul = &Virtual_Mul;
  vtable_.div = &Virtual_Div;
  vtable_.mod = &Virtual_Mod;
  vtable_.greater = &Virtual_Greater;
  vtable_.less = &Virtual_Less;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.le = &Virtual_LessEqual;

  // 设置类名
  set_name(PyString::NewInstance("float"));
}

// static
PyObject* PyFloatKlass::Virtual_Add(PyObject* a, PyObject* b) {}

PyObject* PyFloatKlass::Virtual_Sub(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_Mul(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_Div(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_Mod(PyObject*, PyObject*) {}

PyObject* PyFloatKlass::Virtual_Greater(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_Less(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_Equal(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_NotEqual(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_GreaterEqual(PyObject*, PyObject*) {}
PyObject* PyFloatKlass::Virtual_LessEqual(PyObject*, PyObject*) {}

}  // namespace saauso::internal
