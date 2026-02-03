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
  V(PyTypeObject)               \
  V(PyString)                   \
  V(PyFunction)                 \
  V(PyFloat)                    \
  V(PyBoolean)                  \
  V(PyNone)                     \
  V(PyCodeObject)               \
  V(PyList)                     \
  V(PyListIterator)             \
  V(PyTuple)                    \
  V(PyTupleIterator)            \
  V(PyDict)                     \
  V(PyDictKeys)                 \
  V(PyDictValues)               \
  V(PyDictItems)                \
  V(PyDictKeyIterator)          \
  V(PyDictItemIterator)         \
  V(PyDictValueIterator)        \
  V(FixedArray)                 \
  V(MethodObject)               \
  V(Cell)

#define PY_TYPE_LIST(V) \
  V(PySmi)              \
  PY_TYPE_IN_HEAP_LIST(V)

#define DEFINE_PY_TYPE(name) class name;
PY_TYPE_LIST(DEFINE_PY_TYPE)
#undef DEFINE_PY_TYPE

///////////////////////对象类型检测开始///////////////////////
#define DEFINE_PY_CHECKER(name)           \
  bool Is##name(Tagged<PyObject> object); \
  bool Is##name(Handle<PyObject> object);

PY_TYPE_LIST(DEFINE_PY_CHECKER)
DEFINE_PY_CHECKER(NormalPyFunction)
DEFINE_PY_CHECKER(NativePyFunction)
DEFINE_PY_CHECKER(PyTrue)
DEFINE_PY_CHECKER(PyFalse)
DEFINE_PY_CHECKER(PyNativeFunction)

// 是否在堆区有实体
DEFINE_PY_CHECKER(HeapObject)

// 是否是可以被GC回收或移动的对象
// 排除Smi、布尔值（分配在永久区）、None（分配在永久区）
DEFINE_PY_CHECKER(GcAbleObject)

#undef DEFINE_PY_CHECKER
///////////////////////对象类型检测结束///////////////////////

// **特别提醒**
// PyObject指针内部存储的可能是真正的对象指针，也有可能是一个Smi。
// 如果里面实际存的是一个Smi，会导致C++编译器的UB行为，从而引发
// 编译器胡乱优化而导致的莫名崩溃。
//
// 这意味着，为了规避C++层面的UB行为，我们不能直接传递裸的PyObject*。
// 无论是显式的函数传参传递PyObject*，还是this指针隐式传递
// （通过py_object_ptr->xxx()），都应当避免。
//
// 因此PyObject类当中的所有方法都以static函数的形式提供，
// 并且函数参数为包装后的Handle<PyObject>或Tagged<PyObject>！！！
class PyObject : public Object {
 public:
  static MarkWord GetMarkWord(Tagged<PyObject> object);
  static void SetMapWordForwarded(Tagged<PyObject> object,
                                  Tagged<PyObject> target);

  static Tagged<Klass> GetKlass(Tagged<PyObject> object);
  static Tagged<Klass> GetKlass(Handle<PyObject> object);

  static void SetKlass(Tagged<PyObject> object, Tagged<Klass> klass);
  static void SetKlass(Handle<PyObject> object, Tagged<Klass> klass);

  static Handle<PyDict> GetProperties(Tagged<PyObject> object);
  static Handle<PyDict> GetProperties(Handle<PyObject> object);
  static void SetProperties(Tagged<PyObject> object, Tagged<PyDict> properties);

  static Handle<PyObject> AllocateRawPythonObject();

  static Tagged<PyObject> cast(Tagged<PyObject> object) { return object; }

  ////////////////// 多态函数 开始 //////////////////
  static void Print(Handle<PyObject> self);

  static Handle<PyObject> Add(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Sub(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Mul(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Div(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> FloorDiv(Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Handle<PyObject> Mod(Handle<PyObject> self, Handle<PyObject> other);

  static Handle<PyObject> Len(Handle<PyObject> self);

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

  static bool GreaterBool(Handle<PyObject> self, Handle<PyObject> other);
  static bool LessBool(Handle<PyObject> self, Handle<PyObject> other);
  static bool EqualBool(Handle<PyObject> self, Handle<PyObject> other);
  static bool NotEqualBool(Handle<PyObject> self, Handle<PyObject> other);
  static bool GreaterEqualBool(Handle<PyObject> self, Handle<PyObject> other);
  static bool LessEqualBool(Handle<PyObject> self, Handle<PyObject> other);

  static Handle<PyObject> GetAttr(Handle<PyObject> self,
                                  Handle<PyObject> attr_name,
                                  bool is_try);
  static Handle<PyObject> GetAttrForCall(Handle<PyObject> self,
                                         Handle<PyObject> attr_name,
                                         Handle<PyObject>& self_or_null);
  static void SetAttr(Handle<PyObject> self,
                      Handle<PyObject> attr_name,
                      Handle<PyObject> attr_value);
  static Handle<PyObject> Subscr(Handle<PyObject> self,
                                 Handle<PyObject> subscr_name);

  static void StoreSubscr(Handle<PyObject> self,
                          Handle<PyObject> subscr_name,
                          Handle<PyObject> subscr_value);
  static Tagged<PyBoolean> Contains(Handle<PyObject> self,
                                    Handle<PyObject> target);
  static bool ContainsBool(Handle<PyObject> self, Handle<PyObject> target);
  static Handle<PyObject> Iter(Handle<PyObject> self);
  static Handle<PyObject> Next(Handle<PyObject> self);
  static void DeletSubscr(Handle<PyObject> self, Handle<PyObject> subscr_name);

  static uint64_t Hash(Handle<PyObject> self);

  static Handle<PyObject> Call(Handle<PyObject> self,
                               Handle<PyObject> host,
                               Handle<PyObject> args,
                               Handle<PyObject> kwargs);

  // GC相关接口
  static size_t GetInstanceSize(Tagged<PyObject> self);
  static void Iterate(Tagged<PyObject> self, ObjectVisitor* v);
  ////////////////// 多态函数 结束 //////////////////

 private:
  friend class PyObjectLayoutChecker;

  // 特别提醒：MarkWord必须始终处于Python对象内存布局的开头！！！
  MarkWord mark_word_;

  // PyDict* properties_; python对象的属性容器
  Tagged<PyObject> properties_{nullptr};
};

class PyObjectLayoutChecker {
  // 静态断言：确保PyObject符合虚拟机内存布局的要求
  // 1. 必须是标准布局，保证C++成员变量顺序与内存顺序一致
  static_assert(std::is_standard_layout_v<PyObject>);
  // 2. 不能有虚函数（防止vptr破坏布局）
  static_assert(!std::is_polymorphic_v<PyObject>);
  // 3. MarkWord必须在偏移量0的位置
  static_assert(offsetof(PyObject, mark_word_) == 0);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECTS_H_
