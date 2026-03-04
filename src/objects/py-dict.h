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
  static bool IsDictLike(Tagged<PyObject> object);
  static bool IsDictLike(Handle<PyObject> object);

  int64_t capacity() const;
  Handle<PyObject> KeyAtIndex(int64_t index) const;
  Handle<PyObject> ValueAtIndex(int64_t index) const;
  Handle<PyTuple> ItemAtIndex(int64_t index) const;

  Maybe<bool> ContainsKey(Handle<PyObject> key) const;

  // 根据 key 查 value 系列 API
  // - 返回 true : 成功查询到 value，通过 out 输出。
  // - 返回 false : 未查询到 value，out 输出 null。
  // - 返回空 Maybe : 查询过程中抛出异常，需要通知调用者向上传播异常，out 输出
  //   null。
  Maybe<bool> Get(Handle<PyObject> key, Handle<PyObject>& out) const;
  Maybe<bool> Get(Tagged<PyObject> key, Handle<PyObject>& out) const;
  Maybe<bool> GetTagged(Handle<PyObject> key, Tagged<PyObject>& out) const;
  Maybe<bool> GetTagged(Tagged<PyObject> key, Tagged<PyObject>& out) const;

  Maybe<bool> Remove(Handle<PyObject> key);

  int64_t occupied() const { return occupied_; }

  static Maybe<bool> Put(Handle<PyDict> dict,
                         Handle<PyObject> key,
                         Handle<PyObject> value);
  static Handle<PyTuple> GetKeyTuple(Handle<PyDict> dict);

  Handle<FixedArray> data() const;

 private:
  friend class PyDictKlass;
  friend class Factory;

  static void ExpandImpl(Handle<PyDict> list);
  static Maybe<bool> ExpandImplMaybe(Handle<PyDict> dict);

  // FixedArray* data_
  Tagged<PyObject> data_;

  int64_t occupied_;  // 当前存储的键值对数目
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_H_
