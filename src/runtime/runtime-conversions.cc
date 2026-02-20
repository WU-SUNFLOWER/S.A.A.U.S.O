// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-conversions.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string_view>

#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value) {
  if (IsPySmi(value)) {
    return PySmi::ToInt(Tagged<PySmi>::cast(value));
  }
  if (IsPyBoolean(value)) {
    return Tagged<PyBoolean>::cast(value)->value() ? 1 : 0;
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr,
               "TypeError: '%.*s' object cannot be interpreted as an integer\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
}

}  // namespace saauso::internal
