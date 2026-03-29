// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "src/api/api-impl.h"

namespace saauso {

bool Value::IsString() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyString(object);
}

bool Value::IsInteger() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPySmi(object);
}

bool Value::IsFloat() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  return !object.is_null() && i::IsPyFloat(object);
}

bool Value::IsBoolean() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  i::Isolate* isolate = i::Isolate::Current();
  return i::IsPyTrue(object, isolate) || i::IsPyFalse(object, isolate);
}

Maybe<std::string> Value::ToString() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return i::kNullMaybe;
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  return Maybe<std::string>(
      std::string(py_string->buffer(), static_cast<size_t>(py_string->length())));
}

Maybe<int64_t> Value::ToInteger() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return i::kNullMaybe;
  }
  return Maybe<int64_t>(i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object)));
}

Maybe<double> Value::ToFloat() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return i::kNullMaybe;
  }
  return Maybe<double>(i::Tagged<i::PyFloat>::cast(*object)->value());
}

Maybe<bool> Value::ToBoolean() const {
  i::Handle<i::PyObject> object = i::Utils::OpenHandle(this);
  if (object.is_null()) {
    return i::kNullMaybe;
  }
  i::Isolate* isolate = i::Isolate::Current();
  if (i::IsPyTrue(object, isolate)) {
    return Maybe<bool>(true);
  }
  if (i::IsPyFalse(object, isolate)) {
    return Maybe<bool>(false);
  }
  return i::kNullMaybe;
}

}  // namespace saauso
