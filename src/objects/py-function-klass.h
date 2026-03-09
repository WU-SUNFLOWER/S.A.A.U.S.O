// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FUNCTION_KLASS_H_
#define SAAUSO_OBJECTS_PY_FUNCTION_KLASS_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class PyFunctionKlass : public Klass {
 public:
  static Tagged<PyFunctionKlass> GetInstance();

  PyFunctionKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_Print(Handle<PyObject> self);
  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class NativeFunctionKlass : public Klass {
 public:
  static Tagged<NativeFunctionKlass> GetInstance();

  NativeFunctionKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_Print(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Call(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> host,
                                            Handle<PyObject> args,
                                            Handle<PyObject> kwargs);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class MethodObjectKlass : public Klass {
 public:
  static Tagged<MethodObjectKlass> GetInstance();

  MethodObjectKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_Print(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FUNCTION_KLASS_H_
