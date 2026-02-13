// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime.h"

#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable) {
  HandleScope scope;

  // Fast Path: 直接展开tuple
  if (IsPyTuple(iteratable)) {
    auto tuple = Handle<PyTuple>::cast(iteratable);
    for (int64_t i = 0; i < tuple->length(); ++i) {
      PyList::Append(list, tuple->Get(i));
    }
    return;
  }

  // Fast Path: 直接展开list
  if (IsPyList(iteratable)) {
    auto source = Handle<PyList>::cast(iteratable);
    for (int64_t i = 0; i < source->length(); ++i) {
      PyList::Append(list, source->Get(i));
    }
    return;
  }

  auto iterator = PyObject::Iter(iteratable);
  auto elem = PyObject::Next(iterator);
  while (!elem.is_null()) {
    PyList::Append(list, elem);
    elem = PyObject::Next(iterator);
  }
}

Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable) {
  EscapableHandleScope scope;
  Handle<PyTuple> tuple;
  // Fast Path: List转Tuple
  if (IsPyList(iterable)) {
    auto list = Handle<PyList>::cast(iterable);
    tuple = PyTuple::NewInstance(list->length());
    for (auto i = 0; i < list->length(); ++i) {
      tuple->SetInternal(i, list->Get(i));
    }
    return scope.Escape(tuple);
  }

  // Fallback: 通过迭代器进行转换
  Handle<PyList> tmp = PyList::NewInstance();
  Runtime_ExtendListByItratableObject(tmp, iterable);
  tuple = PyTuple::NewInstance(tmp->length());
  for (auto i = 0; i < tmp->length(); ++i) {
    tuple->SetInternal(i, tmp->Get(i));
  }
  return scope.Escape(tuple);
}

}  // namespace saauso::internal

