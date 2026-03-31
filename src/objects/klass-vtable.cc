// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass-vtable.h"

#include "src/execution/exception-utils.h"
#include "src/objects/klass-vtable-trampolines.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Maybe<void> KlassVtable::Initialize(Isolate* isolate, Tagged<Klass> klass) {
  HandleScope scope;

  InitializeFromSupers(isolate, klass);
  RETURN_ON_EXCEPTION(isolate, UpdateOverrideSlots(isolate, klass));

  return JustVoid();
}

void KlassVtable::Clear() {
#define CLEAR_SLOT(ignore1, slot_name, ignore2, ignore3) slot_name##_ = nullptr;
  KLASS_VTABLE_SLOT_LIST(CLEAR_SLOT)
#undef CLEAR_SLOT
}

void KlassVtable::InitializeFromSupers(Isolate* isolate, Tagged<Klass> klass) {
  // 要求MRO序列必须已经完成初始化
  assert(!klass->mro(isolate).is_null());

  Handle<PyList> mro = klass->mro(isolate);
  for (int64_t i = 1; i < mro->length(); ++i) {
    auto super = Handle<PyTypeObject>::cast(mro->Get(i, isolate));
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

#define COPY_SLOT(ignore1, slot_name, ignore2, ignore3) \
  COPY_SLOT_IMPL(slot_name##_)
  KLASS_VTABLE_SLOT_LIST(COPY_SLOT)
#undef COPY_SLOT

#undef COPY_SLOT_IMPL
#undef CAN_COPY_SLOT
}

Maybe<void> KlassVtable::UpdateOverrideSlots(Isolate* isolate,
                                             Tagged<Klass> klass) {
  Handle<PyObject> dummy;
  Handle<PyDict> klass_properties = klass->klass_properties(isolate);

#define UPDATE_OVERRIDE_SLOT(ignore1, slot_name, ignore2, trampoline_name)   \
  do {                                                                       \
    bool has_magic_method = false;                                           \
    ASSIGN_RETURN_ON_EXCEPTION(                                             \
        isolate, has_magic_method,                                          \
        klass_properties->Get(ST(slot_name, isolate), dummy, isolate));     \
    if (has_magic_method) {                                                  \
      slot_name##_ = &KlassVtableTrampolines::trampoline_name;               \
    }                                                                        \
  } while (0);

  KLASS_VTABLE_SLOT_EXPOSED(UPDATE_OVERRIDE_SLOT)
#undef UPDATE_OVERRIDE_SLOT

  return JustVoid();
}

}  // namespace saauso::internal
