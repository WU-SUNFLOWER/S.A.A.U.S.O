// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-iterator.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

Tagged<PyTupleIterator> PyTupleIterator::cast(Tagged<PyObject> object) {
  assert(IsPyTupleIterator(object));
  return Tagged<PyTupleIterator>::cast(object);
}

Handle<PyTuple> PyTupleIterator::owner() const {
  assert(!owner_.is_null());
  return PyTuple::CastTupleLike(handle(owner_));
}

}  // namespace saauso::internal
