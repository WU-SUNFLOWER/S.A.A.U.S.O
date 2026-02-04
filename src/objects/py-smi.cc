// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi.h"

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string_view>

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"
#include "src/objects/py-string.h"
#include "src/utils/number-conversion.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// 判断一个Tagged<PyObject>是不是Smi
bool PySmi::IsSmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

bool PySmi::IsSmi(Handle<PyObject> object) {
  return IsSmi(*object);
}

// 从一个Tagged<PySmi>中提取出裸的int64_t
int64_t PySmi::ToInt(Tagged<PySmi> smi) {
  assert(smi.IsSmi());
  return AddressToSmi(smi.ptr());
}

int64_t PySmi::ToInt(Handle<PySmi> smi) {
  return ToInt(*smi);
}

// 将一个整型转换成Tagged<PySmi>
Tagged<PySmi> PySmi::FromInt(int64_t value) {
  return Tagged<PySmi>(SmiToAddress(value));
}

Tagged<PySmi> PySmi::FromPyString(Tagged<PyString> py_string) {
  std::string_view s(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
  int64_t parsed = 0;
  StringToIntError err = StringToIntWithBase(s, 10, &parsed);
  if (err != StringToIntError::kOk) {
    if (err == StringToIntError::kOverflow) {
      std::fprintf(stderr, "OverflowError: int too large to convert to Smi\n");
      std::exit(1);
    }
    std::fprintf(stderr,
                 "ValueError: invalid literal for int() with base 10: '%.*s'\n",
                 static_cast<int>(py_string->length()), py_string->buffer());
    std::exit(1);
  }

  if (!InRangeWithRightClose(parsed, kSmiMinValue, kSmiMaxValue)) {
    std::fprintf(stderr, "OverflowError: int too large to convert to Smi\n");
    std::exit(1);
  }
  return FromInt(parsed);
}

// 将裸的Tagged<PyObject>指针转成Tagged<PySmi>
Tagged<PySmi> PySmi::cast(Tagged<PyObject> object) {
  assert(object.IsSmi());
  return Tagged<PySmi>(object.ptr());
}

}  // namespace saauso::internal
