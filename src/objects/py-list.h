// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "handles/handles.h"
#include "objects/py-object.h"

#ifndef SAAUSO_PyObjectS_PY_LIST_H_
#define SAAUSO_PyObjectS_PY_LIST_H_

namespace saauso::internal {

class PyList : public PyObject {
 public:
  static Handle<PyList> NewInstance(int init_capacity = 8);
  static PyList* Cast(PyObject* object);

  // 以下方法均不会触发GC
  // 但为了避免调用方拿到PyObject*裸指针后意外触发GC，导致裸指针变成悬空指针，
  // 我们约定所有返回结果必须使用Handle进行包装。
  // 这样可以强制调用方使用Handle持有堆上对象，缓解悬空指针风险。
  Handle<PyObject> Pop();

  Handle<PyObject> Get(int index) const;
  Handle<PyObject> GetLast() const;

  void Set(int index, Handle<PyObject> value);

  void Remove(int index);
  void Clear();

  int capacity() const { return capacity_; };
  int length() const { return length_; };

  bool IsEmpty() const { return length_ == 0; };
  bool IsFull() const { return length_ == capacity_; }

  // 特别提醒：
  // 调用任何可能会触发 GC（或者参数计算可能触发 GC）的虚函数时，
  // 必须通过 Handle 进行调用（即 -> 的左边必须是一个 Handle 对象）！！！
  void Append(Handle<PyObject> value);
  void Insert(int index, Handle<PyObject> value);

 private:
  int capacity_{0};
  int length_{0};
  // TODO: 实现Visitor入口函数，将PyList和缓冲区拷贝到survivor区
  PyObject** array_{nullptr};

  static void ExpandImpl(Handle<PyList> list);
};

}  // namespace saauso::internal

#endif  // SAAUSO_PyObjectS_PY_LIST_H_
