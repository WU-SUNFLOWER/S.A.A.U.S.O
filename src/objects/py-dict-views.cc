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

Tagged<PyDictKeys> PyDictKeys::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeys(object));
  return Tagged<PyDictKeys>::cast(object);
}

Handle<PyDict> PyDictKeys::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Tagged<PyDictValues> PyDictValues::cast(Tagged<PyObject> object) {
  assert(IsPyDictValues(object));
  return Tagged<PyDictValues>::cast(object);
}

Handle<PyDict> PyDictValues::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Tagged<PyDictItems> PyDictItems::cast(Tagged<PyObject> object) {
  assert(IsPyDictItems(object));
  return Tagged<PyDictItems>::cast(object);
}

Handle<PyDict> PyDictItems::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Tagged<PyDictKeyIterator> PyDictKeyIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeyIterator(object));
  return Tagged<PyDictKeyIterator>::cast(object);
}

Handle<PyDict> PyDictKeyIterator::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
}

Tagged<PyDictValueIterator> PyDictValueIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictValueIterator(object));
  return Tagged<PyDictValueIterator>::cast(object);
}

Handle<PyDict> PyDictValueIterator::owner() const {
  assert(!owner_.is_null());
  return PyDict::CastDictLike(handle(owner_));
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
