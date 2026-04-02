// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-primitive.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/heap/factory.h"
#include "src/objects/py-float.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"

namespace saauso {

Local<String> String::New(Isolate* isolate, std::string_view value) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::Handle<i::PyString> py_string = i::PyString::New(
      i_isolate, value.data(), static_cast<int64_t>(value.size()));

  return api::Utils::ToLocal<String>(py_string);
}

std::string String::Value() const {
  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  assert(!object.is_null() && i::IsPyString(object));

  i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*object);
  return std::string(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));
}

Local<Integer> Integer::New(Isolate* isolate, int64_t value) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::Handle<i::PyObject> smi = i_isolate->factory()->NewSmiFromInt(value);

  return api::Utils::ToLocal<Integer>(smi);
}

int64_t Integer::Value() const {
  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  assert(!object.is_null() && i::IsPySmi(object));

  return i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*object));
}

Local<Float> Float::New(Isolate* isolate, double value) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::Handle<i::PyFloat> py_float = i_isolate->factory()->NewPyFloat(value);

  return api::Utils::ToLocal<Float>(py_float);
}

double Float::Value() const {
  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  assert(!object.is_null() && i::IsPyFloat(object));

  return i::Tagged<i::PyFloat>::cast(*object)->value();
}

Local<Boolean> Boolean::New(Isolate* isolate, bool value) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  i::Handle<i::PyObject> py_bool = i_isolate->factory()->ToPyBoolean(value);

  return api::Utils::ToLocal<Boolean>(py_bool);
}

bool Boolean::Value() const {
  i::Handle<i::PyObject> object = api::Utils::OpenHandle(this);
  assert(!object.is_null());

  i::Isolate* i_isolate = api::RequireCurrentIsolate();
  return i::IsPyTrue(object, i_isolate);
}

}  // namespace saauso
