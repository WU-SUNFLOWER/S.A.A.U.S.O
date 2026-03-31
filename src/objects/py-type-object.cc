// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object-klass.h"

namespace saauso::internal {

// static
Tagged<PyTypeObject> PyTypeObject::cast(Tagged<PyObject> object) {
  assert(IsPyTypeObject(object));
  return Tagged<PyTypeObject>::cast(object);
}

/////////////////////////////////////////////////////////////////////////////////

void PyTypeObject::BindWithKlass(Tagged<Klass> klass, Isolate* isolate) {
  Handle<PyTypeObject> this_handle(Tagged<PyTypeObject>(this), isolate);
  own_klass_ = klass;
  klass->set_type_object(this_handle);
}

Handle<PyList> PyTypeObject::mro(Isolate* isolate) const {
  return own_klass()->mro(isolate);
}

}  // namespace saauso::internal
