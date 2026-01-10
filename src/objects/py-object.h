// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECTS_H_
#define SAAUSO_OBJECTS_PY_OBJECTS_H_

#include <cstddef>

#include "src/handles/handles.h"
#include "src/objects/mark-word.h"
#include "src/objects/objects.h"

namespace saauso::internal {

class Klass;
class ObjectVisitor;
class PyObject;

#define PY_TYPE_IN_HEAP_LIST(V) \
  V(PyFloat)                    \
  V(PyBoolean)                  \
  V(PyNone)                     \
  V(PyCodeObject)               \
  V(PyString)                   \
  V(PyList)

#define PY_TYPE_LIST(V)   \
  PY_TYPE_IN_HEAP_LIST(V) \
  V(PySmi)

#define DEFINE_PY_TYPE(name) class name;
PY_TYPE_LIST(DEFINE_PY_TYPE)
#undef DEFINE_PY_TYPE

///////////////////////对象类型检测开始///////////////////////
#define DEFINE_PY_CHECKER(name)           \
  bool Is##name(Tagged<PyObject> object); \
  bool Is##name(Handle<PyObject> object);

PY_TYPE_LIST(DEFINE_PY_CHECKER)
DEFINE_PY_CHECKER(PyTrue)
DEFINE_PY_CHECKER(PyFalse)

// 是否在堆区有实体
DEFINE_PY_CHECKER(HeapObject)

// 是否是可以被GC回收或移动的对象
// 排除Smi、布尔值（分配在永久区）、None（分配在永久区）
DEFINE_PY_CHECKER(GcAbleObject)

#undef DEFINE_PY_CHECKER
///////////////////////对象类型检测结束///////////////////////

class PyObject : public Object {
 public:
  static Tagged<Klass> GetKlass(Tagged<PyObject> object);
  static Tagged<Klass> GetKlass(Handle<PyObject> object);

  static void SetKlass(Tagged<PyObject> object, Tagged<Klass> klass);
  static void SetKlass(Handle<PyObject> object, Tagged<Klass> klass);

  static MarkWord GetMarkWord(Tagged<PyObject> object);
  static void SetMapWordForwarded(Tagged<PyObject> object);

  ////////////////// 多态函数 开始 //////////////////
  // 特别提醒：
  // 由于PyObject指针内部存储的可能是真正的对象指针，也有可能是一个Smi，
  // 因此我们不能直接传递裸的PyObject*（如果里面实际存的是一个Smi，会导致C++编译器的UB行为）
  // ，无论是显式的函数传参传递PyObject*，还是通过this指针隐式传递。
  // 必须显式地传递包装后的Handle<PyObject>或Tagged<PyObject>！！！
  static void Print(Handle<PyObject> self);

  static Handle<PyObject> Add(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Sub(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Mul(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Div(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Mod(Handle<PyObject> self, Handle<PyObject> other);

  // PyBoolean存活在永久区，不需要Handle进行保护，为了简单起见直接返回裸指针
  static Tagged<PyBoolean> Greater(Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Tagged<PyBoolean> Less(Handle<PyObject> self, Handle<PyObject> other);
  static Tagged<PyBoolean> Equal(Handle<PyObject> self, Handle<PyObject> other);
  static Tagged<PyBoolean> NotEqual(Handle<PyObject> self,
                                    Handle<PyObject> other);
  static Tagged<PyBoolean> GreaterEqual(Handle<PyObject> self,
                                        Handle<PyObject> other);
  static Tagged<PyBoolean> LessEqual(Handle<PyObject> self,
                                     Handle<PyObject> other);

  static Handle<PyObject> GetAttr(Handle<PyObject> self,
                                  Handle<PyObject> attr_name);
  static Handle<PyObject> SetAttr(Handle<PyObject> self,
                                  Handle<PyObject> attr_name,
                                  Handle<PyObject> attr_value);
  static Handle<PyObject> Subscr(Handle<PyObject> self,
                                 Handle<PyObject> subscr_name);

  static void StoreSubscr(Handle<PyObject> self,
                          Handle<PyObject> subscr_name,
                          Handle<PyObject> subscr_value);
  static Tagged<PyBoolean> Contains(Handle<PyObject> self,
                                    Handle<PyObject> target);
  static Handle<PyObject> Iter(Handle<PyObject> self);
  static Handle<PyObject> Next(Handle<PyObject> self);
  static void DeletSubscr(Handle<PyObject> self, Handle<PyObject> subscr_name);

  // GC相关接口
  static size_t GetInstanceSize(Handle<PyObject> self);
  static void Iterate(Handle<PyObject> self, ObjectVisitor* v);
  ////////////////// 多态函数 结束 //////////////////

 private:
  // 特别提醒：MarkWord必须始终处于Python对象内存布局的开头！！！
  MarkWord mark_word_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECTS_H_
