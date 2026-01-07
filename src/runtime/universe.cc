// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/universe.h"

#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"

namespace saauso::internal {

Heap* Universe::heap_ = nullptr;

PyNone* Universe::py_none_object_ = nullptr;
PyBoolean* Universe::py_true_object_ = nullptr;
PyBoolean* Universe::py_false_object_ = nullptr;

// static
void Universe::Genesis() {
  heap_ = new Heap();

  py_none_object_ = PyNone::NewInstance();
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
