// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/string-table.h"

#include "src/objects/py-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

StringTable::StringTable() {
#define INIT_STR_FIELD(name, value) \
  name##_str_ = *PyString::NewInstance(value, true);

#define INIT_STR_FIELD_FOR_EXCEPTION(_, name, value) INIT_STR_FIELD(name, value)

  PY_OBJECT_MAGIC_ATTR_LIST(INIT_STR_FIELD);
  PY_BUILTIN_FUNC_LIST(INIT_STR_FIELD);
  STRING_IN_TABLE_LIST(INIT_STR_FIELD);
  EXCEPTION_TYPE_LIST(INIT_STR_FIELD_FOR_EXCEPTION)

#undef INIT_STR_FIELD_FOR_EXCEPTION
#undef INIT_STR_FIELD
}

void StringTable::Iterate(ObjectVisitor* v) {
#define VISIT_STR_FIELD(name)                     \
  do {                                            \
    Tagged<PyObject> object = name##_str_;        \
    v->VisitPointer(&object);                     \
    name##_str_ = Tagged<PyString>::cast(object); \
  } while (false);

#define VISIT_STR_FIELD_NORMAL(name, _) VISIT_STR_FIELD(name)

#define VISIT_STR_FIELD_FOR_EXCEPTION(ignore1, name, ignore2) \
  VISIT_STR_FIELD(name)

  PY_OBJECT_MAGIC_ATTR_LIST(VISIT_STR_FIELD_NORMAL);
  PY_BUILTIN_FUNC_LIST(VISIT_STR_FIELD_NORMAL);
  STRING_IN_TABLE_LIST(VISIT_STR_FIELD_NORMAL);
  EXCEPTION_TYPE_LIST(VISIT_STR_FIELD_FOR_EXCEPTION);

#undef VISIT_STR_FIELD_FOR_EXCEPTION
#undef VISIT_STR_FIELD_NORMAL
#undef VISIT_STR_FIELD
}

}  // namespace saauso::internal
