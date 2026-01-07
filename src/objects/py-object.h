// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECTS_H_
#define SAAUSO_OBJECTS_PY_OBJECTS_H_

#include "handles/handles.h"
#include "objects/objects.h"

namespace saauso::internal {

class Klass;
class PyBoolean;

class PyObject : public Object {
 public:
  Klass* klass() const;
  void set_klass(Klass* klass);

  // 以下方法依赖对象绑定的klass指针
  bool IsPyFloat() const;
  bool IsPyList() const;
  bool IsPyString() const;
  bool IsPyBoolean() const;

  // 以下方法依赖挂在Universe上的全局指针
  bool IsPyTrue() const;
  bool IsPyFalse() const;
  bool IsPyNone() const;

  // 以下方法依赖打在PyObject*指针上的tag
  bool IsPySmi() const;

  // 是否在堆区有实体，还是只是一个幻影类
  // 排除Smi
  bool IsHeapObject() const;

  // 是否是可以被GC回收或移动的对象
  // 排除Smi、布尔值（分配在永久区）、None（分配在永久区）
  bool IsGcAbleObject() const;

  ////////////////// 多态函数 开始 //////////////////
  // 特别提醒：
  // 调用 PyObject 的任何可能会触发 GC（或者参数计算可能触发 GC）的虚函数时，
  // 必须通过 Handle 进行调用（即 -> 的左边必须是一个 Handle 对象）！！！
  void Print();

  Handle<PyObject> Add(Handle<PyObject> other);
  Handle<PyObject> Sub(Handle<PyObject> other);
  Handle<PyObject> Mul(Handle<PyObject> other);
  Handle<PyObject> Div(Handle<PyObject> other);
  Handle<PyObject> Mod(Handle<PyObject> other);

  // PyBoolean存活在永久区，不需要Handle进行保护，为了简单起见直接返回裸指针
  PyBoolean* Greater(Handle<PyObject> other);
  PyBoolean* Less(Handle<PyObject> other);
  PyBoolean* Equal(Handle<PyObject> other);
  PyBoolean* NotEqual(Handle<PyObject> other);
  PyBoolean* GreaterEqual(Handle<PyObject> other);
  PyBoolean* LessEqual(Handle<PyObject> other);

  Handle<PyObject> GetAttr(Handle<PyObject> attr_name);
  Handle<PyObject> SetAttr(Handle<PyObject> attr_name,
                           Handle<PyObject> attr_value);
  Handle<PyObject> Subscr(Handle<PyObject> subscr_name);

  void StoreSubscr(Handle<PyObject> subscr_name, Handle<PyObject> subscr_value);
  PyBoolean* Contains(Handle<PyObject> target);
  Handle<PyObject> Iter();
  Handle<PyObject> Next();
  void DeletSubscr(Handle<PyObject> subscr_name);

  size_t GetInstanceSize();
  ////////////////// 多态函数 结束 //////////////////

 private:
  Klass* klass_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECTS_H_
