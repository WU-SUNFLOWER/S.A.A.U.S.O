// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_LIST_ITERATOR_H_
#define SAAUSO_OBJECTS_PY_LIST_ITERATOR_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyList;

class PyListIterator : public PyObject {
 public:
  static Tagged<PyListIterator> cast(Tagged<PyObject> object);

  Handle<PyList> owner(Isolate* isolate) const;
  void set_owner(Handle<PyList> owner);
  void set_owner(Tagged<PyList> owner);
  int64_t iter_cnt() const { return iter_cnt_; }
  void set_iter_cnt(int64_t iter_cnt) { iter_cnt_ = iter_cnt; }
  void increase_iter_cnt() { ++iter_cnt_; }

 private:
  friend class PyListIteratorKlass;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_cnt_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_LIST_ITERATOR_H_
