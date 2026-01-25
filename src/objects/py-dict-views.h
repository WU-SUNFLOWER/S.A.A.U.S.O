// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_VIEWS_H_
#define SAAUSO_OBJECTS_PY_DICT_VIEWS_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyDict;

class PyDictKeys : public PyObject {
 public:
  static Handle<PyDictKeys> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictKeys> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;

 private:
  friend class PyDictKeysKlass;

  Tagged<PyObject> owner_{kNullAddress};
};

class PyDictValues : public PyObject {
 public:
  static Handle<PyDictValues> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictValues> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;

 private:
  friend class PyDictValuesKlass;

  Tagged<PyObject> owner_{kNullAddress};
};

class PyDictItems : public PyObject {
 public:
  static Handle<PyDictItems> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictItems> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;

 private:
  friend class PyDictItemsKlass;

  Tagged<PyObject> owner_{kNullAddress};
};

class PyDictKeyIterator : public PyObject {
 public:
  static Handle<PyDictKeyIterator> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictKeyIterator> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;
  int64_t iter_index() const { return iter_index_; }
  void set_iter_index(int64_t iter_index) { iter_index_ = iter_index; }

 private:
  friend class PyDictKeyIteratorKlass;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_index_;
};

class PyDictValueIterator : public PyObject {
 public:
  static Handle<PyDictValueIterator> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictValueIterator> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;
  int64_t iter_index() const { return iter_index_; }
  void set_iter_index(int64_t iter_index) { iter_index_ = iter_index; }

 private:
  friend class PyDictValueIteratorKlass;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_index_;
};

class PyDictItemIterator : public PyObject {
 public:
  static Handle<PyDictItemIterator> NewInstance(Handle<PyObject> owner);
  static Tagged<PyDictItemIterator> cast(Tagged<PyObject> object);

  Handle<PyDict> owner() const;
  int64_t iter_index() const { return iter_index_; }
  void set_iter_index(int64_t iter_index) { iter_index_ = iter_index; }

 private:
  friend class PyDictItemIteratorKlass;

  Tagged<PyObject> owner_{kNullAddress};
  int64_t iter_index_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_VIEWS_H_
