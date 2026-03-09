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
class ObjectVisitor;

namespace {
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
                                                     Tagged<Klass>,
                                                     OopHandle,
                                                     OopHandle);
using VirtualFuncType_Maybe_Init = MaybeOopHandle (*)(Isolate*,
                                                      Tagged<Klass>,
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
}  // namespace

class KlassVtable {
 public:
  KlassVtable() = delete;

  // add/sub/mul/div/floor_div/mod：可能抛 TypeError 等
  VirtualFuncType_Maybe_1_2 add_{nullptr};
  VirtualFuncType_Maybe_1_2 sub_{nullptr};
  VirtualFuncType_Maybe_1_2 mul_{nullptr};
  VirtualFuncType_Maybe_1_2 div_{nullptr};
  VirtualFuncType_Maybe_1_2 floor_div_{nullptr};
  VirtualFuncType_Maybe_1_2 mod_{nullptr};

  VirtualFuncType_MaybeHash hash_{nullptr};

  VirtualFuncType_MaybeGetAttr getattr_{nullptr};
  // setattr/store_subscr/del_subscr：成功返回 None，失败返回空 MaybeHandle
  VirtualFuncType_Maybe_0_3 setattr_{nullptr};
  VirtualFuncType_Maybe_1_2 subscr_{nullptr};
  VirtualFuncType_Maybe_0_3 store_subscr_{nullptr};

  // 比较/contains：返回 Maybe<bool>，失败时设置 pending exception
  VirtualFuncType_MaybeBool_1_2 greater_{nullptr};
  VirtualFuncType_MaybeBool_1_2 less_{nullptr};
  VirtualFuncType_MaybeBool_1_2 equal_{nullptr};
  VirtualFuncType_MaybeBool_1_2 not_equal_{nullptr};
  VirtualFuncType_MaybeBool_1_2 ge_{nullptr};
  VirtualFuncType_MaybeBool_1_2 le_{nullptr};
  VirtualFuncType_MaybeBool_1_2 contains_{nullptr};

  VirtualFuncType_Maybe_1_1 iter_{nullptr};
  VirtualFuncType_Maybe_1_1 next_{nullptr};
  VirtualFuncType_Maybe_Call call_{nullptr};
  VirtualFuncType_Maybe_1_1 len_{nullptr};

  // print：成功返回 None，失败返回空 MaybeHandle
  VirtualFuncType_Maybe_1_1 print_{nullptr};
  VirtualFuncType_Maybe_1_1 repr_{nullptr};
  VirtualFuncType_Maybe_1_2 del_subscr_{nullptr};

  // 获取实例对象大小，调用该函数绝对不允许触发GC
  size_t (*instance_size_)(Oop){nullptr};

  VirtualFuncType_Maybe_New new_instance_{nullptr};
  VirtualFuncType_Maybe_Init init_instance_{nullptr};

  // 扫描对象内部数据，用于GC
  void (*iterate_)(Oop, ObjectVisitor*){nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_VTABLE_H_
