// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_H_
#define SAAUSO_OBJECTS_PY_DICT_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class Isolate;

class PyDict : public PyObject {
 public:
  static constexpr int64_t kMinimumCapacity = 8;

  static Handle<PyDict> New(Isolate* isolate,int64_t init_capacity = kMinimumCapacity);

  static Tagged<PyDict> cast(Tagged<PyObject> object);

  int64_t capacity() const;
  Handle<PyObject> KeyAtIndex(int64_t index) const;
  Handle<PyObject> ValueAtIndex(int64_t index) const;
  Handle<PyTuple> ItemAtIndex(int64_t index, Isolate* isolate) const;

  Maybe<bool> ContainsKey(Handle<PyObject> key, Isolate* isolate) const;

  // 根据 key 查 value 系列 API
  // - 返回 true : 成功查询到 value，通过 out 输出。
  // - 返回 false : 未查询到 value，out 输出 null。
  // - 返回空 Maybe : 查询过程中抛出异常，需要通知调用者向上传播异常，out 输出
  //   null。
  Maybe<bool> Get(Handle<PyObject> key,
                  Handle<PyObject>& out,
                  Isolate* isolate) const;
  Maybe<bool> Get(Tagged<PyObject> key,
                  Handle<PyObject>& out,
                  Isolate* isolate) const;
  Maybe<bool> GetTagged(Handle<PyObject> key,
                        Tagged<PyObject>& out,
                        Isolate* isolate) const;
  Maybe<bool> GetTagged(Tagged<PyObject> key,
                        Tagged<PyObject>& out,
                        Isolate* isolate) const;

  Maybe<bool> Remove(Handle<PyObject> key, Isolate* isolate);

  int64_t occupied() const { return occupied_; }

  static Maybe<bool> Put(Handle<PyDict> dict,
                         Handle<PyObject> key,
                         Handle<PyObject> value,
                         Isolate* isolate);
  static Handle<PyTuple> GetKeyTuple(Handle<PyDict> dict, Isolate* isolate);

  Handle<FixedArray> data() const;

 private:
  friend class PyDictKlass;
  friend class Factory;

  static Maybe<bool> ExpandImpl(Handle<PyDict> dict, Isolate* isolate);

  // FixedArray* data_
  Tagged<PyObject> data_;

  int64_t occupied_;  // 当前存储的键值对数目
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_H_
