// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_VIEWS_KLASS_H_
#define SAAUSO_OBJECTS_PY_DICT_VIEWS_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyTuple;

class PyDictKeysKlass : public Klass {
 public:
  static Tagged<PyDictKeysKlass> GetInstance();

  PyDictKeysKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Contains(Handle<PyObject> self,
                                            Handle<PyObject> subscr);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class PyDictValuesKlass : public Klass {
 public:
  static Tagged<PyDictValuesKlass> GetInstance();

  PyDictValuesKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Contains(Handle<PyObject> self,
                                            Handle<PyObject> subscr);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class PyDictItemsKlass : public Klass {
 public:
  static Tagged<PyDictItemsKlass> GetInstance();

  PyDictItemsKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Contains(Handle<PyObject> self,
                                            Handle<PyObject> subscr);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class PyDictKeyIteratorKlass : public Klass {
 public:
  static Tagged<PyDictKeyIteratorKlass> GetInstance();

  PyDictKeyIteratorKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

  static Handle<PyObject> Native_Next(Handle<PyObject> self,
                                      Handle<PyTuple> args,
                                      Handle<PyDict> kwargs);

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Next(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class PyDictItemIteratorKlass : public Klass {
 public:
  static Tagged<PyDictItemIteratorKlass> GetInstance();

  PyDictItemIteratorKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

  static Handle<PyObject> Native_Next(Handle<PyObject> self,
                                      Handle<PyTuple> args,
                                      Handle<PyDict> kwargs);

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Next(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

class PyDictValueIteratorKlass : public Klass {
 public:
  static Tagged<PyDictValueIteratorKlass> GetInstance();

  PyDictValueIteratorKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

  static Handle<PyObject> Native_Next(Handle<PyObject> self,
                                      Handle<PyTuple> args,
                                      Handle<PyDict> kwargs);

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Next(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_VIEWS_KLASS_H_
