// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_
#define SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_

#include "src/objects/klass.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyObjectKlass : public Klass {
 public:
  static Tagged<PyObjectKlass> GetInstance();

  PyObjectKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

  static Maybe<uint64_t> Generic_Hash(Handle<PyObject> self);

  static Maybe<bool> Generic_GetAttr(Handle<PyObject> self,
                                     Handle<PyObject> prop_name,
                                     bool is_try,
                                     Handle<PyObject>& out_prop_val);

  static MaybeHandle<PyObject> Generic_GetAttrForCall(
      Handle<PyObject> self,
      Handle<PyObject> prop_name,
      Handle<PyObject>& self_or_null);

  static MaybeHandle<PyObject> Generic_SetAttr(Handle<PyObject> self,
                                               Handle<PyObject> property_name,
                                               Handle<PyObject> property_value);

  static MaybeHandle<PyObject> Generic_Call(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> host,
                                            Handle<PyObject> args,
                                            Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Generic_NewInstance(Isolate* isolate,
                                                   Tagged<Klass> klass_self,
                                                   Handle<PyObject> args,
                                                   Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Generic_InitInstance(Isolate* isolate,
                                                    Tagged<Klass> klass_self,
                                                    Handle<PyObject> instance,
                                                    Handle<PyObject> args,
                                                    Handle<PyObject> kwargs);

  static void Generic_Iterate(Tagged<PyObject>, ObjectVisitor*);

  static size_t Generic_InstanceSize(Tagged<PyObject> self);

  static MaybeHandle<PyObject> Generic_Print(Handle<PyObject> self);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_
