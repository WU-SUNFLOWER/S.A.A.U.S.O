// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-iterator.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict-iterator-klass.h"
#include "src/objects/py-dict.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

Handle<PyDictIterator> PyDictIterator::NewInstance(Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictIterator> iterator(Isolate::Current()->heap()->Allocate<
                                  PyDictIterator>(Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;
  iterator->iter_index_ = 0;

  PyObject::SetKlass(iterator, PyDictIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::Null());

  return iterator.EscapeFrom(&scope);
}

Tagged<PyDictIterator> PyDictIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictIterator(object));
  return Tagged<PyDictIterator>::cast(object);
}

Handle<PyDict> PyDictIterator::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

}  // namespace saauso::internal
