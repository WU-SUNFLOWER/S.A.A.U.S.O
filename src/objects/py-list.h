// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_PyObjectS_PY_LIST_H_
#define SAAUSO_PyObjectS_PY_LIST_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyList : public PyObject {
 public:
  static constexpr int64_t kDefaultInitialCapacity = 4;
  static Handle<PyList> NewInstance(
      int64_t init_capacity = kDefaultInitialCapacity);
  static Tagged<PyList> Cast(Tagged<PyObject> object);

  // 以下方法均不会触发GC
  // 但为了避免调用方拿到Tagged<PyObject>裸指针后意外触发GC，导致裸指针变成悬空指针，
  // 我们约定所有返回结果必须使用Handle进行包装。
  // 这样可以强制调用方使用Handle持有堆上对象，缓解悬空指针风险。
  Handle<PyObject> Pop();

  Handle<PyObject> Get(int index) const;
  Handle<PyObject> GetLast() const;

  void Set(int64_t index, Handle<PyObject> value);

  void Remove(int64_t index);
  void Clear();

  int64_t capacity() const { return capacity_; };
  int64_t length() const { return length_; };

  bool IsEmpty() const { return length_ == 0; };
  bool IsFull() const {
    assert(length_ <= capacity_);
    return length_ == capacity_;
  }

  void** data_slot_address() { return reinterpret_cast<void**>(&array_); }

  // 特别提醒：
  // 调用任何可能会触发 GC（或者参数计算可能触发 GC）的虚函数时，
  // 必须通过 Handle 进行调用（即 -> 的左边必须是一个 Handle 对象）！！！
  static void Append(Handle<PyList> self, Handle<PyObject> value);
  static void Insert(Handle<PyList> self,
                     int64_t index,
                     Handle<PyObject> value);

 private:
  friend class PyListKlass;

  int64_t capacity_{0};
  int64_t length_{0};
  // TODO: 实现Visitor入口函数，将PyList和缓冲区拷贝到survivor区
  Tagged<PyObject>* array_{nullptr};

  static void ExpandImpl(Handle<PyList> list);
};

}  // namespace saauso::internal

#endif  // SAAUSO_PyObjectS_PY_LIST_H_
