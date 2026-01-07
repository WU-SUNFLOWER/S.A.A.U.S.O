// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "runtime/universe.h"

#include "heap/heap.h"
#include "objects/klass.h"
#include "objects/py-float-klass.h"
#include "objects/py-list-klass.h"
#include "objects/py-oddballs-klass.h"
#include "objects/py-oddballs.h"
#include "objects/py-smi-klass.h"
#include "objects/py-string-klass.h"

namespace saauso::internal {

Heap* Universe::heap_ = nullptr;

PyObject* Universe::py_none_object_ = nullptr;
PyBoolean* Universe::py_true_object_ = nullptr;
PyBoolean* Universe::py_false_object_ = nullptr;

// static
void Universe::Genesis() {
  heap_ = new Heap();

  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);

  PyNoneKlass::GetInstance()->Initialize();
  PyBooleanKlass::GetInstance()->Initialize();
  PyFloatKlass::GetInstance()->Initialize();
  PySmiKlass::GetInstance()->Initialize();
  PyStringKlass::GetInstance()->Initialize();
  PyListKlass::GetInstance()->Initialize();
}

// static
void Universe::Destroy() {}

}  // namespace saauso::internal
