// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-primitive.h"
#include "src/api/api-impl.h"

namespace saauso {

Local<String> String::New(Isolate* isolate, std::string_view value) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  return api::WrapHostString<String>(i_isolate, std::string(value));
}

std::string String::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyString(object)) {
    return "";
  }
  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  return std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
}

Local<Integer> Integer::New(Isolate* isolate, int64_t value) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  return api::WrapHostInteger<Integer>(i_isolate, value);
}

int64_t Integer::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPySmi(object)) {
    return 0;
  }
  return i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object));
}

Local<Float> Float::New(Isolate* isolate, double value) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  return api::WrapHostFloat<Float>(i_isolate, value);
}

double Float::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null() || !i::IsPyFloat(object)) {
    return 0.0;
  }
  return i::Tagged<i::PyFloat>::cast(*object)->value();
}

Local<Boolean> Boolean::New(Isolate* isolate, bool value) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  assert(i_isolate == i::Isolate::Current());

  return api::WrapHostBoolean<Boolean>(i_isolate, value);
}

bool Boolean::Value() const {
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(this);
  if (object.is_null()) {
    return false;
  }
  return i::IsPyTrue(object);
}

}  // namespace saauso
