// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-iterator.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

Handle<PyListIterator> PyListIterator::NewInstance(Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyListIterator> iterator(
      Isolate::Current()->heap()->Allocate<PyListIterator>(
          Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
  iterator->iter_cnt_ = 0;

  PyObject::SetKlass(iterator, PyListIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::null());

  return iterator.EscapeFrom(&scope);
}

Tagged<PyListIterator> PyListIterator::cast(Tagged<PyObject> object) {
  assert(IsPyListIterator(object));
  return Tagged<PyListIterator>::cast(object);
}

Handle<PyList> PyListIterator::owner() const {
  assert(!owner_.is_null());
  return handle(Tagged<PyList>::cast(owner_));
}

}  // namespace saauso::internal
