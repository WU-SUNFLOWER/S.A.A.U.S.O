// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_H_
#define SAAUSO_OBJECTS_PY_DICT_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyDict : public PyObject {
 public:
  static constexpr int64_t kMinimumCapacity = 8;

  static Handle<PyDict> NewInstance(int64_t init_capacity = kMinimumCapacity);
  static Tagged<PyDict> cast(Tagged<PyObject> object);

  int64_t capacity() const;
  Handle<PyObject> KeyAtIndex(int64_t index) const;
  Handle<PyObject> ValueAtIndex(int64_t index) const;
  Handle<PyObject> ItemAtIndex(int64_t index) const;

  Tagged<PyBoolean> Contains(Handle<PyObject> key) const;

  Handle<PyObject> Get(Handle<PyObject> key) const;

  void Remove(Handle<PyObject> key);

  int64_t occupied() const { return occupied_; }

  static void Put(Handle<PyObject> dict,
                  Handle<PyObject> key,
                  Handle<PyObject> value);
  static Handle<PyTuple> GetKeyTuple(Handle<PyDict> dict);

 private:
  friend class PyDictKlass;

  static void ExpandImpl(Handle<PyDict> list);

  Handle<FixedArray> data() const;

  // FixedArray* data_
  Tagged<PyObject> data_;

  int64_t occupied_;  // 当前存储的键值对数目
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_H_
