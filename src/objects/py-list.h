// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_PyObjectS_PY_LIST_H_
#define SAAUSO_PyObjectS_PY_LIST_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class FixedArray;

class PyList : public PyObject {
 public:
  static constexpr int64_t kMinimumCapacity = 2;

  static Handle<PyList> NewInstance(int64_t init_capacity = kMinimumCapacity);
  static Tagged<PyList> cast(Tagged<PyObject> object);

  // 以下方法均不会触发GC
  // 但为了避免调用方拿到Tagged<PyObject>裸指针后意外触发GC，导致裸指针变成悬空指针，
  // 我们约定所有返回结果必须使用Handle进行包装。
  // 这样可以强制调用方使用Handle持有堆上对象，缓解悬空指针风险。
  Handle<PyObject> Pop();

  Handle<PyObject> Get(int64_t index) const;
  Handle<PyObject> GetLast() const;

  void Set(int64_t index, Handle<PyObject> value);

  void RemoveByIndex(int64_t index);
  // 删除**第一个**与target匹配的元素
  void Remove(Handle<PyObject> target);
  void Clear();

  int64_t capacity() const;
  int64_t length() const { return length_; };

  bool IsEmpty() const { return length_ == 0; };
  bool IsFull() const {
    assert(length_ <= capacity());
    return length_ == capacity();
  }

  // 查找**第一个**匹配数组元素的下标
  int64_t IndexOf(Handle<PyObject> target) const;

  static void Append(Handle<PyList> self, Handle<PyObject> value);
  static void Insert(Handle<PyList> self,
                     int64_t index,
                     Handle<PyObject> value);

  Tagged<FixedArray> array() const { return Tagged<FixedArray>::cast(array_); }

 private:
  friend class PyListKlass;

  static void ExpandImpl(Handle<PyList> list);

  int64_t length_{0};
  // FixedArray* array_
  Tagged<PyObject> array_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_PyObjectS_PY_LIST_H_
