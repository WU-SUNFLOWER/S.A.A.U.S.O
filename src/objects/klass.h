// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_H_
#define SAAUSO_OBJECTS_KLASS_H_

#include <cstddef>

#include "src/handles/handles.h"
#include "src/objects/objects.h"

namespace saauso::internal {

class PyObject;
class PyString;
class PyTypeObject;
class PyBoolean;
class ObjectVisitor;

/////////////////虚函数表 定义开始///////////////////////////

struct VirtualTable {
  using Oop = Tagged<PyObject>;
  using OopHandle = Handle<PyObject>;

  using VirtualFuncType_0_1 = void (*)(Oop);
  using VirtualFuncType_1_1 = Oop (*)(Oop);
  using VirtualFuncType_0_2 = void (*)(Oop, Oop);
  using VirtualFuncType_1_2 = Oop (*)(Oop, Oop);
  using VirtualFuncType_0_3 = void (*)(Oop, Oop, Oop);
  using VirtualFuncType_1_3 = Oop (*)(Oop, Oop, Oop);

  using VirtualFuncType_0_1_SAFE = void (*)(OopHandle);
  using VirtualFuncType_1_1_SAFE = OopHandle (*)(OopHandle);
  using VirtualFuncType_0_2_SAFE = void (*)(OopHandle, OopHandle);
  using VirtualFuncType_1_2_SAFE = OopHandle (*)(OopHandle, OopHandle);
  using VirtualFuncType_0_3_SAFE = void (*)(OopHandle, OopHandle, OopHandle);
  using VirtualFuncType_1_3_SAFE = OopHandle (*)(OopHandle,
                                                 OopHandle,
                                                 OopHandle);

  using VirtualFuncType_1_1_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle);
  using VirtualFuncType_1_2_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle,
                                                              OopHandle);
  using VirtualFuncType_1_3_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle,
                                                              OopHandle,
                                                              OopHandle);

  VirtualTable();

  // Tagged<PyObject> add(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE add{nullptr};
  // Tagged<PyObject> sub(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE sub{nullptr};
  // Tagged<PyObject> mul(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE mul{nullptr};
  // Tagged<PyObject> div(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE div{nullptr};

  uint64_t (*hash)(OopHandle){nullptr};

  // Tagged<PyObject> getattr(Tagged<PyObject> object, Tagged<PyObject> key);
  VirtualFuncType_1_2_SAFE getattr{nullptr};
  // Tagged<PyObject> setattr(Tagged<PyObject> object, Tagged<PyObject> key,
  // Tagged<PyObject> value);
  VirtualFuncType_0_3_SAFE setattr{nullptr};
  // Tagged<PyObject> subscr(Tagged<PyObject> object, Tagged<PyObject> subscr);
  VirtualFuncType_1_2_SAFE subscr{nullptr};
  // void store_subscr(Tagged<PyObject> object, Tagged<PyObject> subscr,
  // Tagged<PyObject> value);
  VirtualFuncType_0_3_SAFE store_subscr{nullptr};

  // Tagged<PyBoolean> greater(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL greater{nullptr};
  // Tagged<PyBoolean> less(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL less{nullptr};
  // Tagged<PyBoolean> equal(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL equal{nullptr};
  // Tagged<PyBoolean> not_equal(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL not_equal{nullptr};
  // Tagged<PyBoolean> ge(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL ge{nullptr};
  // Tagged<PyBoolean> le(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL le{nullptr};

  // Tagged<PyObject> mod(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE mod{nullptr};

  // Tagged<PyObject> iter(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE iter{nullptr};
  // Tagged<PyObject> next(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE next{nullptr};
  // Tagged<PyObject> call(Tagged<PyObject> object, Tagged<PyList> args, PyDict*
  // kwargs);
  VirtualFuncType_1_3_SAFE call{nullptr};
  // Tagged<PyObject> len(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE len{nullptr};

  // Tagged<PyObject> contains(Tagged<PyObject> object);
  VirtualFuncType_1_2_SAFE_BOOL contains{nullptr};

  // void print(Tagged<PyObject> object);
  VirtualFuncType_0_1_SAFE print{nullptr};
  // Tagged<PyObject> repr(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE repr{nullptr};
  // void del_subscr(Tagged<PyObject> object, Tagged<PyObject> subscr);
  VirtualFuncType_0_2_SAFE del_subscr{nullptr};

  // 获取实例对象大小，调用该函数绝对不允许触发GC
  // size_t instance_object(Tagged<PyObject>);
  size_t (*instance_size)(Oop){nullptr};

  // 扫描对象内部数据，用于GC
  void (*iterate)(Oop, ObjectVisitor*){nullptr};
};
/////////////////虚函数表 定义结束///////////////////////////

class Klass : public Object {
 public:
  Klass() = default;

  Handle<PyString> name();
  void set_name(Handle<PyString> name);

  Handle<PyTypeObject> type_object();
  void set_type_object(Handle<PyTypeObject>);

  // Klass被视为一种GC ROOT，这是暴露给GC的接口
  void Iterate(ObjectVisitor* v);

  // Python对象虚函数表
  VirtualTable vtable_;

  // 默认虚函数
  static void Virtual_Default_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Default_Len(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Default_Repr(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Default_Call(Handle<PyObject> self,
                                               Handle<PyObject> args,
                                               Handle<PyObject> kwargs);
  static Handle<PyObject> Virtual_Default_GetAttr(
      Handle<PyObject> self,
      Handle<PyObject> property_name);
  static void Virtual_Default_SetAttr(Handle<PyObject> self,
                                      Handle<PyObject> property_name,
                                      Handle<PyObject> property_value);
  static Handle<PyObject> Virtual_Default_Subscr(Handle<PyObject> self,
                                                 Handle<PyObject> subscr);
  static void Virtual_Default_StoreSubscr(Handle<PyObject> self,
                                          Handle<PyObject> subscr,
                                          Handle<PyObject> value);
  static void Virtual_Default_Delete_Subscr(Handle<PyObject> self,
                                            Handle<PyObject> subscr);
  static Handle<PyObject> Virtual_Default_Add(Handle<PyObject> self,
                                              Handle<PyObject> other);
  static Handle<PyObject> Virtual_Default_Next(Handle<PyObject> self);
  static size_t Virtual_Default_InstanceSize(Tagged<PyObject> self);

 private:
  // PyString* name_ 类型名，如"str"、"list"、"dict"等
  Tagged<PyObject> name_{kNullAddress};

  // PyTypeObject* klass类型在python世界对应的type对象
  Tagged<PyObject> type_object_{kNullAddress};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_H_
