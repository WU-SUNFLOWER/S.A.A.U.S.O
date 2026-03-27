// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-iterator.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list.h"

namespace saauso::internal {

Tagged<PyListIterator> PyListIterator::cast(Tagged<PyObject> object) {
  assert(IsPyListIterator(object));
  return Tagged<PyListIterator>::cast(object);
}

Handle<PyList> PyListIterator::owner() const {
  assert(!owner_.is_null());
  return handle(Tagged<PyList>::cast(owner_));
}

}  // namespace saauso::internal
