// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TUPLE_ITERATOR_H_
#define SAAUSO_OBJECTS_PY_TUPLE_ITERATOR_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyTuple;

class PyTupleIterator : public PyObject {
 public:
  static Tagged<PyTupleIterator> cast(Tagged<PyObject> object);

  Handle<PyTuple> owner(Isolate* isolate) const;
  int64_t iter_cnt() const { return iter_cnt_; }
  void increase_iter_cnt() { ++iter_cnt_; }

 private:
  friend class PyTupleIteratorKlass;
  friend class Factory;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_cnt_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_TUPLE_ITERATOR_H_
