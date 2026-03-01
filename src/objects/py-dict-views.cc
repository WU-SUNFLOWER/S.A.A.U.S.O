// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-views.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict.h"

namespace saauso::internal {

Handle<PyDictKeys> PyDictKeys::NewInstance(Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictKeys(owner);
}

Tagged<PyDictKeys> PyDictKeys::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeys(object));
  return Tagged<PyDictKeys>::cast(object);
}

Handle<PyDict> PyDictKeys::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Handle<PyDictValues> PyDictValues::NewInstance(Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictValues(owner);
}

Tagged<PyDictValues> PyDictValues::cast(Tagged<PyObject> object) {
  assert(IsPyDictValues(object));
  return Tagged<PyDictValues>::cast(object);
}

Handle<PyDict> PyDictValues::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Handle<PyDictItems> PyDictItems::NewInstance(Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictItems(owner);
}

Tagged<PyDictItems> PyDictItems::cast(Tagged<PyObject> object) {
  assert(IsPyDictItems(object));
  return Tagged<PyDictItems>::cast(object);
}

Handle<PyDict> PyDictItems::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Handle<PyDictKeyIterator> PyDictKeyIterator::NewInstance(
    Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictKeyIterator(owner);
}

Tagged<PyDictKeyIterator> PyDictKeyIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeyIterator(object));
  return Tagged<PyDictKeyIterator>::cast(object);
}

Handle<PyDict> PyDictKeyIterator::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Handle<PyDictValueIterator> PyDictValueIterator::NewInstance(
    Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictValueIterator(owner);
}

Tagged<PyDictValueIterator> PyDictValueIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictValueIterator(object));
  return Tagged<PyDictValueIterator>::cast(object);
}

Handle<PyDict> PyDictValueIterator::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Handle<PyDictItemIterator> PyDictItemIterator::NewInstance(
    Handle<PyObject> owner) {
  return Isolate::Current()->factory()->NewPyDictItemIterator(owner);
}

Tagged<PyDictItemIterator> PyDictItemIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictItemIterator(object));
  return Tagged<PyDictItemIterator>::cast(object);
}

Handle<PyDict> PyDictItemIterator::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

}  // namespace saauso::internal
