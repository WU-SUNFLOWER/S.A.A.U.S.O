// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "src/api/api-impl.h"

namespace saauso {

bool Value::IsString() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyString(object);
}

bool Value::IsInteger() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPySmi(object);
}

bool Value::IsFloat() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyFloat(object);
}

bool Value::IsBoolean() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyTrue(object) || i::IsPyFalse(object);
}

bool Value::ToString(std::string* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return false;
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  *out = std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
  return true;
}

bool Value::ToInteger(int64_t* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return false;
  }
  *out = i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object));
  return true;
}

bool Value::ToFloat(double* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return false;
  }
  *out = i::Tagged<i::PyFloat>::cast(*object)->value();
  return true;
}

bool Value::ToBoolean(bool* out) const {
  if (out == nullptr) {
    return false;
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  if (i::IsPyTrue(object)) {
    *out = true;
    return true;
  }
  if (i::IsPyFalse(object)) {
    *out = false;
    return true;
  }
  return false;
}

}  // namespace saauso
