// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-iterator.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

Handle<PyTupleIterator> PyTupleIterator::NewInstance(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  Handle<PyTupleIterator> iterator(
      Isolate::Current()->heap()->Allocate<PyTupleIterator>(
          Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
  iterator->iter_cnt_ = 0;

  PyObject::SetKlass(iterator, PyTupleIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::null());

  return scope.Escape(iterator);
}

Tagged<PyTupleIterator> PyTupleIterator::cast(Tagged<PyObject> object) {
  assert(IsPyTupleIterator(object));
  return Tagged<PyTupleIterator>::cast(object);
}

Handle<PyTuple> PyTupleIterator::owner() const {
  assert(!owner_.is_null());
  return handle(Tagged<PyTuple>::cast(owner_));
}

}  // namespace saauso::internal
