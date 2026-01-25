// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_ITERATOR_H_
#define SAAUSO_OBJECTS_PY_DICT_ITERATOR_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyDict;

class PyDictIterator : public PyObject {
 public:
  static Handle<PyDictIterator> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictIterator> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;
  int64_t iter_index() const { return iter_index_; }
  void set_iter_index(int64_t iter_index) { iter_index_ = iter_index; }

 private:
  friend class PyDictIteratorKlass;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_index_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_ITERATOR_H_
