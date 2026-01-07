// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_STRING_KLASS_H_
#define SAAUSO_OBJECTS_PY_STRING_KLASS_H_

#include "objects/klass.h"
#include "handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyBoolean;

class PyStringKlass : public Klass {
 public:
  static PyStringKlass* GetInstance();

  void Initialize();

 private:
  PyStringKlass();
  static PyStringKlass* instance_;

  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);

  static PyBoolean* Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other);
  static PyBoolean* Virtual_NotEqual(Handle<PyObject> self, Handle<PyObject> other);
  static PyBoolean* Virtual_Less(Handle<PyObject> self, Handle<PyObject> other);
  static PyBoolean* Virtual_Greater(Handle<PyObject> self, Handle<PyObject> other);
  static PyBoolean* Virtual_LessEqual(Handle<PyObject> self, Handle<PyObject> other);
  static PyBoolean* Virtual_GreaterEqual(Handle<PyObject> self, Handle<PyObject> other);

  static Handle<PyObject> Virtual_Subscr(Handle<PyObject> self, Handle<PyObject> subscr);
  static Handle<PyObject> Virtual_Add(Handle<PyObject> self, Handle<PyObject> other);
  static void Virtual_Print(Handle<PyObject> self);
  static size_t Virtual_InstanceSize(PyObject* self);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_STRING_KLASS_H_
