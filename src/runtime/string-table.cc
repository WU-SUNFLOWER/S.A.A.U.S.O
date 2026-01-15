// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/string-table.h"

#include "src/objects/visitors.h"

namespace saauso::internal {

StringTable::StringTable() {
#define INIT_STR_FIELD(name, value) \
  name##_str_ = *PyString::NewInstance(value, true);
  STRING_IN_TABLE_LIST(INIT_STR_FIELD);
  PY_OBJECT_MAGIC_ATTR_LIST(INIT_STR_FIELD);
#undef INIT_STR_FIELD
}

void StringTable::Iterate(ObjectVisitor* v) {
#define VISIT_STR_FIELD(name, _) \
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&name##_str_));
  STRING_IN_TABLE_LIST(VISIT_STR_FIELD);
  PY_OBJECT_MAGIC_ATTR_LIST(VISIT_STR_FIELD);
#undef VISIT_STR_FIELD
}

}  // namespace saauso::internal
