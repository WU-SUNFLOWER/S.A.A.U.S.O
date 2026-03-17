// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_VTABLE_TRAMPOLINES_H_
#define SAAUSO_OBJECTS_KLASS_VTABLE_TRAMPOLINES_H_

#include "src/common/globals.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class KlassVtableTrampolines : public AllStatic {
 public:
  static MaybeHandle<PyObject> Add(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> Sub(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> Mul(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> Div(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> FloorDiv(Handle<PyObject> self,
                                        Handle<PyObject> other);

  static MaybeHandle<PyObject> Mod(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static Maybe<uint64_t> Hash(Handle<PyObject> self);

  static Maybe<bool> GetAttr(Handle<PyObject> self,
                             Handle<PyObject> prop_name,
                             bool is_try,
                             Handle<PyObject>& out_prop_val);

  static MaybeHandle<PyObject> SetAttr(Handle<PyObject> self,
                                       Handle<PyObject> property_name,
                                       Handle<PyObject> property_value);

  static MaybeHandle<PyObject> Subscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr);

  static MaybeHandle<PyObject> StoreSubscr(Handle<PyObject> self,
                                           Handle<PyObject> subscr,
                                           Handle<PyObject> value);

  static MaybeHandle<PyObject> DeleteSubscr(Handle<PyObject> self,
                                            Handle<PyObject> subscr);

  static Maybe<bool> Greater(Handle<PyObject> self, Handle<PyObject> other);

  static Maybe<bool> Less(Handle<PyObject> self, Handle<PyObject> other);

  static Maybe<bool> Equal(Handle<PyObject> self, Handle<PyObject> other);

  static Maybe<bool> NotEqual(Handle<PyObject> self, Handle<PyObject> other);

  static Maybe<bool> GreaterEqual(Handle<PyObject> self,
                                  Handle<PyObject> other);

  static Maybe<bool> LessEqual(Handle<PyObject> self, Handle<PyObject> other);

  static Maybe<bool> Contains(Handle<PyObject> self, Handle<PyObject> other);

  static MaybeHandle<PyObject> Iter(Handle<PyObject> self);

  static MaybeHandle<PyObject> Next(Handle<PyObject> self);

  static MaybeHandle<PyObject> Call(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> receiver,
                                    Handle<PyObject> args,
                                    Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Len(Handle<PyObject> self);

  static MaybeHandle<PyObject> Repr(Handle<PyObject> self);

  static MaybeHandle<PyObject> Str(Isolate* isolate, Handle<PyObject> self);

  static MaybeHandle<PyObject> NewInstance(Isolate* isolate,
                                           Handle<PyTypeObject> receiver_type,
                                           Handle<PyObject> args,
                                           Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> InitInstance(Isolate* isolate,
                                            Handle<PyObject> instance,
                                            Handle<PyObject> args,
                                            Handle<PyObject> kwargs);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_VTABLE_TRAMPOLINES_H_
