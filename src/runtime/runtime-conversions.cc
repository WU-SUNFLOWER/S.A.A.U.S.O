// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-conversions.h"

#include <cmath>
#include <limits>
#include <string_view>

#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

Maybe<int64_t> Runtime_DecodeIntLike(Tagged<PyObject> value) {
  if (IsPySmi(value)) {
    return Maybe<int64_t>(PySmi::ToInt(Tagged<PySmi>::cast(value)));
  }
  if (IsPyBoolean(value)) {
    return Maybe<int64_t>(Tagged<PyBoolean>::cast(value)->value() ? 1 : 0);
  }

  auto type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "'%s' object cannot be interpreted as an integer",
                      type_name->buffer());

  return kNullMaybe;
}

}  // namespace saauso::internal
