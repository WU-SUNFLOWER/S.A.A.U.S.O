// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass-vtable.h"

#include "src/execution/exception-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

KlassVtable kDefaultVtable = {
    .add_ = &Klass::Virtual_Default_Add,
    .sub_ = &Klass::Virtual_Default_Sub,
    .mul_ = &Klass::Virtual_Default_Mul,
    .div_ = &Klass::Virtual_Default_Div,
    .floor_div_ = &Klass::Virtual_Default_FloorDiv,
    .mod_ = &Klass::Virtual_Default_Mod,
    .hash_ = &Klass::Virtual_Default_Hash,

    .getattr_ = &Klass::Virtual_Default_GetAttr,
    .setattr_ = &Klass::Virtual_Default_SetAttr,
    .subscr_ = &Klass::Virtual_Default_Subscr,
    .store_subscr_ = &Klass::Virtual_Default_StoreSubscr,
    .del_subscr_ = &Klass::Virtual_Default_Delete_Subscr,

    .greater_ = &Klass::Virtual_Default_Greater,
    .less_ = &Klass::Virtual_Default_Less,
    .equal_ = &Klass::Virtual_Default_Equal,
    .not_equal_ = &Klass::Virtual_Default_NotEqual,
    .ge_ = &Klass::Virtual_Default_GreaterEqual,
    .le_ = &Klass::Virtual_Default_LessEqual,
    .contains_ = &Klass::Virtual_Default_Contains,

    .iter_ = &Klass::Virtual_Default_Iter,
    .next_ = &Klass::Virtual_Default_Next,

    .call_ = &Klass::Virtual_Default_Call,
    .len_ = &Klass::Virtual_Default_Len,
    .repr_ = &Klass::Virtual_Default_Repr,
    .str_ = &Klass::Virtual_Default_Str,

    .new_instance_ = &Klass::Virtual_Default_NewInstance,
    .init_instance_ = &Klass::Virtual_Default_InitInstance,

    .print_ = &Klass::Virtual_Default_Print,
    .instance_size_ = &Klass::Virtual_Default_InstanceSize,
    .iterate_ = &Klass::Virtual_Default_Iterate,
};

}  // namespace

Maybe<void> KlassVtable::Initialize(Isolate* isolate, Tagged<Klass> klass) {
  HandleScope scope;

  // 后续算法依赖槽位值是否为nullptr来判断槽位是否被填充。
  // 由于不保证分配器分配出来的内存已经置零，因此首先需要显式清空所有槽位。
  // Clear();

  InitializeFromSupers(klass);
  RETURN_ON_EXCEPTION(isolate, UpdateOverrideSlots(isolate, klass));

  return JustVoid();
}

void KlassVtable::Clear() {
#define CLEAR_SLOT(ignore1, slot_name, ignore2) slot_name##_ = nullptr;
  KLASS_VTABLE_SLOT_LIST(CLEAR_SLOT)
#undef CLEAR_SLOT
}

void KlassVtable::InitializeFromSupers(Tagged<Klass> klass) {
  // 要求MRO序列必须已经完成初始化
  assert(!klass->mro().is_null());

  Handle<PyList> mro = klass->mro();
  for (int64_t i = 1; i < mro->length(); ++i) {
    auto super = Handle<PyTypeObject>::cast(mro->Get(i));
    auto super_klass = super->own_klass();
    CopyInheritedSlotsFromSuper(super_klass);
  }
}

void KlassVtable::CopyInheritedSlotsFromSuper(Tagged<Klass> super_klass) {
  const KlassVtable* super_vtable = &super_klass->vtable();
  Tagged<Klass> super_base = super_klass->native_layout_base();
  const KlassVtable* super_base_vtable = nullptr;
  if (!super_base.is_null()) {
    super_base_vtable = &super_base->vtable();
  }

#define CAN_COPY_SLOT(slot)         \
  (super_vtable->slot != nullptr && \
   (super_base.is_null() || super_vtable->slot != super_base_vtable->slot))

#define COPY_SLOT_IMPL(slot)                    \
  if (slot == nullptr && CAN_COPY_SLOT(slot)) { \
    slot = super_vtable->slot;                  \
  }

#define COPY_SLOT(ignore1, slot_name, ignore2) COPY_SLOT_IMPL(slot_name##_)
  KLASS_VTABLE_SLOT_LIST(COPY_SLOT)
#undef COPY_SLOT

#undef COPY_SLOT_IMPL
#undef CAN_COPY_SLOT
}

Maybe<void> KlassVtable::UpdateOverrideSlots(Isolate* isolate,
                                             Tagged<Klass> klass) {
  Handle<PyDict> properties = klass->klass_properties();

#define UPDATE_OVERRIDE_SLOT(ignore1, slot_name, ignore2)               \
  do {                                                                  \
    bool has_magic_method = false;                                      \
    ASSIGN_RETURN_ON_EXCEPTION(isolate, has_magic_method,               \
                               properties->ContainsKey(ST(slot_name))); \
    if (has_magic_method) {                                             \
      slot_name##_ = kDefaultVtable.slot_name##_;                       \
    }                                                                   \
  } while (0);

  KLASS_VTABLE_SLOT_EXPOSED(UPDATE_OVERRIDE_SLOT)
#undef UPDATE_OVERRIDE_SLOT

  return JustVoid();
}  // namespace saauso::internal

}  // namespace saauso::internal
