// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_LIST_H_
#define SAAUSO_OBJECTS_PY_LIST_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class Isolate;

class FixedArray;

class PyList : public PyObject {
 public:
  static constexpr int64_t kMinimumCapacity = 2;
  static constexpr int kNotFound = -1;

  static Handle<PyList> New(Isolate* isolate,
                            int64_t init_capacity = kMinimumCapacity);

  static Tagged<PyList> cast(Tagged<PyObject> object);

  Handle<PyObject> Pop(Isolate* isolate);
  Tagged<PyObject> PopTagged();

  Handle<PyObject> Get(int64_t index, Isolate* isolate) const;
  Tagged<PyObject> GetTagged(int64_t index) const;

  Handle<PyObject> GetLast(Isolate* isolate) const;
  Tagged<PyObject> GetLastTagged() const;

  // 常规的set操作，要求0<=index<length
  void Set(int64_t index, Handle<PyObject> value);
  // 虚拟机内部的set操作，要求0<=index<capacity。
  // 如果length<=index<capacity，会强制更新length（中间空洞的内容不确定，严禁访问！）
  // 此操作仅限虚拟机内部使用，严禁泄露给上层python代码！
  void SetAndExtendLength(int64_t index, Handle<PyObject> value);

  void RemoveByIndex(int64_t index);

  // 删除**第一个**与target匹配的元素
  // - 返回 true : 在List中查到target并成功删除。
  // - 返回 false : 未在List中查到target。
  // - 返回空 Maybe : 在List中查找target时出现异常。
  Maybe<bool> Remove(Handle<PyObject> target, Isolate* isolate);

  void Clear();

  int64_t capacity() const;
  int64_t length() const { return length_; }
  void set_length(int64_t length) { length_ = length; }

  bool IsEmpty() const { return length_ == 0; };
  bool IsFull() const {
    assert(length_ <= capacity());
    return length_ == capacity();
  }

  // 查找**第一个**与target匹配元素的下标
  // 返回kNotFound：未找到目标元素
  // 返回非kNotFound的有效下标：找到目标元素
  // 返回空Maybe：查找过程中发生异常，需要调用方向上传播异常
  Maybe<int64_t> IndexOf(Handle<PyObject> target, Isolate* isolate) const;
  Maybe<int64_t> IndexOf(Handle<PyObject> target,
                         int64_t begin,
                         int64_t end,
                         Isolate* isolate) const;

  static void Append(Handle<PyList> self,
                     Handle<PyObject> value,
                     Isolate* isolate);
  static void Insert(Handle<PyList> self,
                     int64_t index,
                     Handle<PyObject> value,
                     Isolate* isolate);

  Tagged<FixedArray> array() const { return Tagged<FixedArray>::cast(array_); }
  void set_array(Handle<FixedArray> array);
  void set_array(Tagged<FixedArray> array);

  static void ExtendByItratableObject(Handle<PyList> list,
                                      Handle<PyObject> source);

 private:
  friend class PyListKlass;

  static void ExpandImpl(Handle<PyList> list, Isolate* isolate);

  int64_t length_{0};
  // FixedArray* array_
  Tagged<PyObject> array_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_LIST_H_
