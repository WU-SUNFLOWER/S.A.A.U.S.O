// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_H_
#define SAAUSO_OBJECTS_KLASS_H_

#include <cstddef>
#include <cstdint>

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/objects.h"
#include "src/utils/maybe.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Klass;
class PyObject;
class PyString;
class PyTypeObject;
class PyDict;
class PyList;
class PyTuple;
class PyBoolean;
class ObjectVisitor;

/////////////////虚函数表 定义开始///////////////////////////

struct VirtualTable {
  using Oop = Tagged<PyObject>;
  using OopHandle = Handle<PyObject>;

  VirtualTable() = delete;

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
  using VirtualFuncType_1_4_SAFE = OopHandle (*)(OopHandle,
                                                 OopHandle,
                                                 OopHandle,
                                                 OopHandle);

  using VirtualFuncType_1_1_SAFE_CBOOL = bool (*)(OopHandle);
  using VirtualFuncType_1_2_SAFE_CBOOL = bool (*)(OopHandle, OopHandle);
  using VirtualFuncType_1_3_SAFE_CBOOL = bool (*)(OopHandle,
                                                  OopHandle,
                                                  OopHandle);

  // Fallible slot：返回 MaybeHandle<PyObject>，失败时设置 pending exception
  using MaybeOopHandle = MaybeHandle<PyObject>;
  using VirtualFuncType_Maybe_1_1 = MaybeOopHandle (*)(OopHandle);
  using VirtualFuncType_Maybe_1_2 = MaybeOopHandle (*)(OopHandle, OopHandle);
  using VirtualFuncType_Maybe_0_3 = MaybeOopHandle (*)(OopHandle,
                                                       OopHandle,
                                                       OopHandle);
  using VirtualFuncType_Maybe_1_4 = MaybeOopHandle (*)(OopHandle,
                                                       OopHandle,
                                                       OopHandle,
                                                       OopHandle);
  using VirtualFuncType_Maybe_Construct = MaybeOopHandle (*)(Tagged<Klass>,
                                                             OopHandle,
                                                             OopHandle);
  // Fallible slot：返回 Maybe<bool>，避免 false 与异常二义性
  using VirtualFuncType_MaybeBool_1_2 = Maybe<bool> (*)(OopHandle, OopHandle);
  // getattr：true=命中（out 写入值），false=未命中（out 为 null），Nothing=异常
  using VirtualFuncType_MaybeGetAttr = Maybe<bool> (*)(OopHandle,
                                                       OopHandle,
                                                       bool,
                                                       OopHandle&);
  // Fallible slot：返回 Maybe<uint64_t>
  using VirtualFuncType_MaybeHash = Maybe<uint64_t> (*)(OopHandle);

  // add/sub/mul/div/floor_div/mod：可能抛 TypeError 等
  VirtualFuncType_Maybe_1_2 add{nullptr};
  VirtualFuncType_Maybe_1_2 sub{nullptr};
  VirtualFuncType_Maybe_1_2 mul{nullptr};
  VirtualFuncType_Maybe_1_2 div{nullptr};
  VirtualFuncType_Maybe_1_2 floor_div{nullptr};
  VirtualFuncType_Maybe_1_2 mod{nullptr};

  VirtualFuncType_MaybeHash hash{nullptr};

  VirtualFuncType_MaybeGetAttr getattr{nullptr};
  // setattr/store_subscr/del_subscr：成功返回 None，失败返回空 MaybeHandle
  VirtualFuncType_Maybe_0_3 setattr{nullptr};
  VirtualFuncType_Maybe_1_2 subscr{nullptr};
  VirtualFuncType_Maybe_0_3 store_subscr{nullptr};

  // 比较/contains：返回 Maybe<bool>，失败时设置 pending exception
  VirtualFuncType_MaybeBool_1_2 greater{nullptr};
  VirtualFuncType_MaybeBool_1_2 less{nullptr};
  VirtualFuncType_MaybeBool_1_2 equal{nullptr};
  VirtualFuncType_MaybeBool_1_2 not_equal{nullptr};
  VirtualFuncType_MaybeBool_1_2 ge{nullptr};
  VirtualFuncType_MaybeBool_1_2 le{nullptr};
  VirtualFuncType_MaybeBool_1_2 contains{nullptr};

  VirtualFuncType_Maybe_1_1 iter{nullptr};
  VirtualFuncType_Maybe_1_1 next{nullptr};
  VirtualFuncType_Maybe_1_4 call{nullptr};
  VirtualFuncType_Maybe_1_1 len{nullptr};

  // print：成功返回 None，失败返回空 MaybeHandle
  VirtualFuncType_Maybe_1_1 print{nullptr};
  VirtualFuncType_Maybe_1_1 repr{nullptr};
  VirtualFuncType_Maybe_1_2 del_subscr{nullptr};

  // 获取实例对象大小，调用该函数绝对不允许触发GC
  size_t (*instance_size)(Oop){nullptr};

  VirtualFuncType_Maybe_Construct construct_instance{nullptr};

  // 扫描对象内部数据，用于GC
  void (*iterate)(Oop, ObjectVisitor*){nullptr};
};
/////////////////虚函数表 定义结束///////////////////////////

enum class NativeLayoutKind : uint8_t {
  kPyObject = 0,
  kList = 1,
  kDict = 2,
  kTuple = 3,
  kString = 4,
};

class Klass : public Object {
 public:
  static Tagged<Klass> CreateRawPythonKlass();

  Klass() = delete;

  void InitializeVTable();

  Handle<PyString> name();
  void set_name(Handle<PyString> name);

  Handle<PyTypeObject> type_object();
  void set_type_object(Handle<PyTypeObject>);

  Handle<PyDict> klass_properties();
  void set_klass_properties(Handle<PyDict>);

  Handle<PyList> supers();
  void set_supers(Handle<PyList>);

  Handle<PyList> mro();
  void set_mro(Handle<PyList> mro);

  NativeLayoutKind native_layout_kind() const { return native_layout_kind_; }
  void set_native_layout_kind(NativeLayoutKind kind) {
    native_layout_kind_ = kind;
  }

  Tagged<Klass> native_layout_base() const { return native_layout_base_; }
  void set_native_layout_base(Tagged<Klass> base) {
    native_layout_base_ = base;
  }

  void CopyVTableFrom(Tagged<Klass> base);

  const VirtualTable& vtable() const { return vtable_; }

  // Klass被视为一种GC ROOT，这是暴露给GC的接口
  void Iterate(ObjectVisitor* v);

  // 添加父类
  void AddSuper(Tagged<Klass> super);
  // 执行C3算法，执行结果保存在mro_
  void OrderSupers();

  // 创建一个对象实例。失败时返回空 MaybeHandle 并已设置 pending exception。
  MaybeHandle<PyObject> ConstructInstance(Handle<PyObject> args,
                                          Handle<PyObject> kwargs);

  // 默认虚函数（Fallible 均返回 MaybeHandle<PyObject> 或 Maybe<bool>）
  static MaybeHandle<PyObject> Virtual_Default_Print(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Default_Len(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Default_Repr(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Default_Call(Handle<PyObject> self,
                                                    Handle<PyObject> host,
                                                    Handle<PyObject> args,
                                                    Handle<PyObject> kwargs);
  static Maybe<bool> Virtual_Default_GetAttr(Handle<PyObject> self,
                                             Handle<PyObject> prop_name,
                                             bool is_try,
                                             Handle<PyObject>& out_prop_val);
  static MaybeHandle<PyObject> Virtual_Default_GetAttrForCall(
      Handle<PyObject> self,
      Handle<PyObject> property_name,
      Handle<PyObject>& self_or_null);
  static MaybeHandle<PyObject> Virtual_Default_SetAttr(
      Handle<PyObject> self,
      Handle<PyObject> property_name,
      Handle<PyObject> property_value);
  static MaybeHandle<PyObject> Virtual_Default_Subscr(Handle<PyObject> self,
                                                      Handle<PyObject> subscr);
  static MaybeHandle<PyObject> Virtual_Default_StoreSubscr(
      Handle<PyObject> self,
      Handle<PyObject> subscr,
      Handle<PyObject> value);
  static MaybeHandle<PyObject> Virtual_Default_Delete_Subscr(
      Handle<PyObject> self,
      Handle<PyObject> subscr);
  static MaybeHandle<PyObject> Virtual_Default_Add(Handle<PyObject> self,
                                                   Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_Greater(Handle<PyObject> self,
                                             Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_Less(Handle<PyObject> self,
                                          Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_Equal(Handle<PyObject> self,
                                           Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_NotEqual(Handle<PyObject> self,
                                              Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_GreaterEqual(Handle<PyObject> self,
                                                  Handle<PyObject> other);
  static Maybe<bool> Virtual_Default_LessEqual(Handle<PyObject> self,
                                               Handle<PyObject> other);

  static MaybeHandle<PyObject> Virtual_Default_ConstructInstance(
      Tagged<Klass> klass_self,
      Handle<PyObject> args,
      Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Virtual_Default_Next(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Default_Iter(Handle<PyObject> object);

  static Maybe<uint64_t> Virtual_Default_Hash(Handle<PyObject> self);

  static void Virtual_Default_Iterate(Tagged<PyObject>, ObjectVisitor*);
  static size_t Virtual_Default_InstanceSize(Tagged<PyObject> self);

 protected:
  // Python对象虚函数表
  VirtualTable vtable_;

  // PyDict* klass_properties_
  Tagged<PyObject> klass_properties_{kNullAddress};

 private:
  // PyString* name_ 类型名，如"str"、"list"、"dict"等
  Tagged<PyObject> name_{kNullAddress};

  // PyTypeObject* klass类型在python世界对应的type对象
  Tagged<PyObject> type_object_{kNullAddress};

  // PyList* supers_
  // 当前klass对应的python类型继承自哪几种类型
  Tagged<PyObject> supers_{kNullAddress};
  // C3算法的运行结果
  Tagged<PyObject> mro_{kNullAddress};
  
  // 内建内存布局信息
  Tagged<Klass> native_layout_base_{kNullAddress};
  NativeLayoutKind native_layout_kind_{NativeLayoutKind::kPyObject};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_H_
