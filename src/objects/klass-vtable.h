// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_VTABLE_H_
#define SAAUSO_OBJECTS_KLASS_VTABLE_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class Klass;
class PyObject;
class PyTypeObject;
class ObjectVisitor;

namespace {
using Oop = Tagged<PyObject>;
using OopHandle = Handle<PyObject>;
using OopTypeObjectHandle = Handle<PyTypeObject>;

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
using VirtualFuncType_1_3_SAFE = OopHandle (*)(OopHandle, OopHandle, OopHandle);
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
using VirtualFuncType_Maybe_Call = MaybeOopHandle (*)(Isolate* isolate,
                                                      OopHandle,
                                                      OopHandle,
                                                      OopHandle,
                                                      OopHandle);

using VirtualFuncType_Maybe_New = MaybeOopHandle (*)(Isolate*,
                                                     OopTypeObjectHandle,
                                                     OopHandle,
                                                     OopHandle);
using VirtualFuncType_Maybe_Init = MaybeOopHandle (*)(Isolate* isolate,
                                                      OopHandle,
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

using VirtualFuncType_InstanceSize = size_t (*)(Oop);

using VirtualFuncType_Iterate = void (*)(Oop, ObjectVisitor*);
}  // namespace

#define KLASS_VTABLE_SLOT_EXPOSED(V)                                           \
  /* add/sub/mul/div/floor_div/mod/hash：可能抛 TypeError 等*/                 \
  V(VirtualFuncType_Maybe_1_2, add, "__add__", Add)                            \
  V(VirtualFuncType_Maybe_1_2, sub, "__sub__", Sub)                            \
  V(VirtualFuncType_Maybe_1_2, mul, "__mul__", Mul)                            \
  V(VirtualFuncType_Maybe_1_2, div, "__truediv__", Div)                        \
  V(VirtualFuncType_Maybe_1_2, floor_div, "__floordiv__", FloorDiv)            \
  V(VirtualFuncType_Maybe_1_2, mod, "__mod__", Mod)                            \
  V(VirtualFuncType_MaybeHash, hash, "__hash__", Hash)                         \
                                                                               \
  /* setattr/store_subscr/del_subscr：成功返回 None，失败返回空 MaybeHandle */ \
  V(VirtualFuncType_MaybeGetAttr, getattr, "__getattr__", GetAttr)             \
  V(VirtualFuncType_Maybe_0_3, setattr, "__setattr__", SetAttr)                \
  V(VirtualFuncType_Maybe_1_2, subscr, "__getitem__", Subscr)                  \
  V(VirtualFuncType_Maybe_0_3, store_subscr, "__setitem__", StoreSubscr)       \
  V(VirtualFuncType_Maybe_1_2, del_subscr, "__delitem__", DeleteSubscr)        \
                                                                               \
  /* 比较/contains：返回 Maybe<bool>，失败时设置 pending exception */          \
  V(VirtualFuncType_MaybeBool_1_2, greater, "__gt__", Greater)                 \
  V(VirtualFuncType_MaybeBool_1_2, less, "__lt__", Less)                       \
  V(VirtualFuncType_MaybeBool_1_2, equal, "__eq__", Equal)                     \
  V(VirtualFuncType_MaybeBool_1_2, not_equal, "__ne__", NotEqual)              \
  V(VirtualFuncType_MaybeBool_1_2, ge, "__ge__", GreaterEqual)                 \
  V(VirtualFuncType_MaybeBool_1_2, le, "__le__", LessEqual)                    \
  V(VirtualFuncType_MaybeBool_1_2, contains, "__contains__", Contains)         \
                                                                               \
  V(VirtualFuncType_Maybe_1_1, iter, "__iter__", Iter)                         \
  V(VirtualFuncType_Maybe_1_1, next, "__next__", Next)                         \
                                                                               \
  V(VirtualFuncType_Maybe_Call, call, "__call__", Call)                        \
  V(VirtualFuncType_Maybe_1_1, len, "__len__", Len)                            \
  V(VirtualFuncType_Maybe_1_1, repr, "__repr__", Repr)                         \
  V(VirtualFuncType_Maybe_1_1, str, "__str__", Str)                            \
                                                                               \
  V(VirtualFuncType_Maybe_New, new_instance, "__new__", NewInstance)           \
  V(VirtualFuncType_Maybe_Init, init_instance, "__init__", InitInstance)

// TODO:
// print这个虚函数在原版CPython中并不存在，需要移除并替换成repr+str的打印模式
#define KLASS_VTABLE_SLOT_INTERNAL(V)                              \
  V(VirtualFuncType_Maybe_1_1, print, "", Print)                   \
  /* 获取实例对象大小，调用该函数绝对不允许触发GC */               \
  V(VirtualFuncType_InstanceSize, instance_size, "", InstanceSize) \
  /* 扫描对象内部数据，用于GC */                                   \
  V(VirtualFuncType_Iterate, iterate, "", Iterate)

#define KLASS_VTABLE_SLOT_LIST(V) \
  KLASS_VTABLE_SLOT_EXPOSED(V)    \
  KLASS_VTABLE_SLOT_INTERNAL(V)

class KlassVtable {
 public:
  KlassVtable() = default;

  Maybe<void> Initialize(Isolate* isolate, Tagged<Klass> klass);

  void Clear();

#define DEFINE_VTABLE_SLOT(signature, field_name, ignore1, ignore2) \
  signature field_name##_{nullptr};
  KLASS_VTABLE_SLOT_LIST(DEFINE_VTABLE_SLOT)
#undef DEFINE_VTABLE_SLOT

 private:
  void InitializeFromSupers(Tagged<Klass> klass);

  void CopyInheritedSlotsFromSuper(Tagged<Klass> super_klass);

  Maybe<void> UpdateOverrideSlots(Isolate* isolate, Tagged<Klass> klass);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_VTABLE_H_
