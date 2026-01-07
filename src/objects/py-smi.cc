// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi.h"

namespace saauso::internal {

int64_t PySmi::value() {
  return reinterpret_cast<int64_t>(this) >> kSmiTagSize;
}

// static
PySmi* PySmi::FromInt(int64_t value) {
  return reinterpret_cast<PySmi*>((value << kSmiTagSize) | kSmiTag);
}

// static
PySmi* PySmi::Cast(PyObject* object) {
  assert(object->IsPySmi());
  return reinterpret_cast<PySmi*>(object);
}

}  // namespace saauso::internal
