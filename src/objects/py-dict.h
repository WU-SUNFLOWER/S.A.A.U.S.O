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
  static Handle<PyDict> Clone(Handle<PyDict> other);

  static Tagged<PyDict> cast(Tagged<PyObject> object);

  int64_t capacity() const;
  Handle<PyObject> KeyAtIndex(int64_t index) const;
  Handle<PyObject> ValueAtIndex(int64_t index) const;
  Handle<PyTuple> ItemAtIndex(int64_t index) const;

  Maybe<bool> ContainsMaybe(Handle<PyObject> key) const;

  Maybe<Tagged<PyObject>> GetTaggedMaybe(Handle<PyObject> key) const;
  Maybe<Tagged<PyObject>> GetTaggedMaybe(Tagged<PyObject> key) const;
  Maybe<bool> RemoveMaybe(Handle<PyObject> key);

  int64_t occupied() const { return occupied_; }

  static Maybe<bool> PutMaybe(Handle<PyDict> dict,
                              Handle<PyObject> key,
                              Handle<PyObject> value);
  static Handle<PyTuple> GetKeyTuple(Handle<PyDict> dict);

  Handle<FixedArray> data() const;

 private:
  friend class PyDictKlass;

  static Handle<PyDict> NewInstanceWithoutAllocateData();

  static void ExpandImpl(Handle<PyDict> list);
  static Maybe<bool> ExpandImplMaybe(Handle<PyDict> dict);

  // FixedArray* data_
  Tagged<PyObject> data_;

  int64_t occupied_;  // 当前存储的键值对数目
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_H_
