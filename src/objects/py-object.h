// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECTS_H_
#define SAAUSO_OBJECTS_PY_OBJECTS_H_

#include <cstddef>

#include "include/saauso-maybe.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/mark-word.h"
#include "src/objects/object-checkers.h"
#include "src/objects/objects.h"

namespace saauso::internal {

class Klass;
class ObjectVisitor;
class PyObject;

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

  static Tagged<Klass> GetHeapKlassUnchecked(Tagged<PyObject> object);

  static void SetKlass(Tagged<PyObject> object, Tagged<Klass> klass);
  static void SetKlass(Handle<PyObject> object, Tagged<Klass> klass);

  static Handle<PyString> GetTypeName(Tagged<PyObject> object,
                                      Isolate* isolate);
  static Handle<PyString> GetTypeName(Handle<PyObject> object,
                                      Isolate* isolate);

  static Handle<PyDict> GetProperties(Tagged<PyObject> object);
  static Handle<PyDict> GetProperties(Handle<PyObject> object);
  static void SetProperties(Tagged<PyObject> object, Tagged<PyDict> properties);

  static Tagged<PyObject> cast(Tagged<PyObject> object) { return object; }

  ////////////////// 多态函数 开始 //////////////////
  static MaybeHandle<PyObject> Repr(Isolate* isolate, Handle<PyObject> self);
  static MaybeHandle<PyObject> Str(Isolate* isolate, Handle<PyObject> self);

  static MaybeHandle<PyObject> Add(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);
  static MaybeHandle<PyObject> Sub(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);
  static MaybeHandle<PyObject> Mul(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);
  static MaybeHandle<PyObject> Div(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);
  static MaybeHandle<PyObject> FloorDiv(Isolate* isolate,
                                        Handle<PyObject> self,
                                        Handle<PyObject> other);
  static MaybeHandle<PyObject> Mod(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> Len(Isolate* isolate, Handle<PyObject> self);

  // 比较：失败时返回空 MaybeHandle 并已设置 pending exception。
  static MaybeHandle<PyBoolean> Greater(Isolate* isolate,
                                        Handle<PyObject> self,
                                        Handle<PyObject> other);
  static MaybeHandle<PyBoolean> Less(Isolate* isolate,
                                     Handle<PyObject> self,
                                     Handle<PyObject> other);
  static MaybeHandle<PyBoolean> Equal(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> other);
  static MaybeHandle<PyBoolean> NotEqual(Isolate* isolate,
                                         Handle<PyObject> self,
                                         Handle<PyObject> other);
  static MaybeHandle<PyBoolean> GreaterEqual(Isolate* isolate,
                                             Handle<PyObject> self,
                                             Handle<PyObject> other);
  static MaybeHandle<PyBoolean> LessEqual(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other);

  // Bool 版比较：返回 Maybe<bool>，避免 false 与异常二义性。
  static Maybe<bool> GreaterBool(Isolate* isolate,
                                 Handle<PyObject> self,
                                 Handle<PyObject> other);
  static Maybe<bool> LessBool(Isolate* isolate,
                              Handle<PyObject> self,
                              Handle<PyObject> other);
  static Maybe<bool> EqualBool(Isolate* isolate,
                               Handle<PyObject> self,
                               Handle<PyObject> other);
  static Maybe<bool> NotEqualBool(Isolate* isolate,
                                  Handle<PyObject> self,
                                  Handle<PyObject> other);
  static Maybe<bool> GreaterEqualBool(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> other);
  static Maybe<bool> LessEqualBool(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);

  // Lookup：
  // - 返回 true 表示查询成功，out 为查询结果（非null）；
  // - 返回 false 表示查询失败，out 为 null
  // - 返回空 Maybe 表示查询过程中出现异常，需要调用方继续向上传播；out 为 null
  static Maybe<bool> LookupAttr(Isolate* isolate,
                                Handle<PyObject> self,
                                Handle<PyObject> attr_name,
                                Handle<PyObject>& out);
  // Fallible：属性不存在或查询异常时返回空 MaybeHandle 并已设置 pending
  // exception。
  static MaybeHandle<PyObject> GetAttr(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> attr_name);
  static MaybeHandle<PyObject> GetAttrForCall(Isolate* isolate,
                                              Handle<PyObject> self,
                                              Handle<PyObject> attr_name,
                                              Handle<PyObject>& self_or_null);
  // 成功返回 None，失败返回空 MaybeHandle。
  static MaybeHandle<PyObject> SetAttr(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> attr_name,
                                       Handle<PyObject> attr_value);

  static MaybeHandle<PyObject> Subscr(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> subscr_name);
  static MaybeHandle<PyObject> StoreSubscr(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> subscr_name,
                                           Handle<PyObject> subscr_value);
  static MaybeHandle<PyObject> DeletSubscr(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> subscr_name);

  static MaybeHandle<PyBoolean> Contains(Isolate* isolate,
                                         Handle<PyObject> self,
                                         Handle<PyObject> target);
  static Maybe<bool> ContainsBool(Isolate* isolate,
                                  Handle<PyObject> self,
                                  Handle<PyObject> target);
  static MaybeHandle<PyObject> Iter(Isolate* isolate, Handle<PyObject> self);
  // 返回空 MaybeHandle 表示迭代结束或异常；调用方需用
  // Runtime_ConsumePendingStopIterationIfSet 区分。
  static MaybeHandle<PyObject> Next(Isolate* isolate, Handle<PyObject> self);

  static Maybe<uint64_t> Hash(Isolate* isolate, Handle<PyObject> self);

  static MaybeHandle<PyObject> Call(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> receiver,
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
