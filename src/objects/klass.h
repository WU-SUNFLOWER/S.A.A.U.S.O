// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_H_
#define SAAUSO_OBJECTS_KLASS_H_

#include "src/handles/handles.h"
#include "src/objects/objects.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class PyBoolean;

/////////////////虚函数表 定义开始///////////////////////////

struct VirtualTable {
  using Oop = PyObject*;
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
  using VirtualFuncType_1_3_SAFE = OopHandle (*)(OopHandle,
                                                 OopHandle,
                                                 OopHandle);

  using VirtualFuncType_1_1_SAFE_BOOL = PyBoolean* (*)(OopHandle);
  using VirtualFuncType_1_2_SAFE_BOOL = PyBoolean* (*)(OopHandle, OopHandle);
  using VirtualFuncType_1_3_SAFE_BOOL = PyBoolean* (*)(OopHandle,
                                                       OopHandle,
                                                       OopHandle);

  // PyObject* add(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE add{nullptr};
  // PyObject* sub(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE sub{nullptr};
  // PyObject* mul(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE mul{nullptr};
  // PyObject* div(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE div{nullptr};

  // PyObject* getattr(PyObject* object, PyObject* key);
  VirtualFuncType_1_2_SAFE getattr{nullptr};
  // PyObject* setattr(PyObject* object, PyObject* key, PyObject* value);
  VirtualFuncType_1_3_SAFE setattr{nullptr};
  // PyObject* subscr(PyObject* object, PyObject* subscr);
  VirtualFuncType_1_2_SAFE subscr{nullptr};
  // void store_subscr(PyObject* object, PyObject* subscr, PyObject* value);
  VirtualFuncType_0_3_SAFE store_subscr{nullptr};

  // PyBoolean* greater(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL greater{nullptr};
  // PyBoolean* less(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL less{nullptr};
  // PyBoolean* equal(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL equal{nullptr};
  // PyBoolean* not_equal(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL not_equal{nullptr};
  // PyBoolean* ge(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL ge{nullptr};
  // PyBoolean* le(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE_BOOL le{nullptr};

  // PyObject* mod(PyObject* a, PyObject* b);
  VirtualFuncType_1_2_SAFE mod{nullptr};

  // PyObject* iter(PyObject* object);
  VirtualFuncType_1_1_SAFE iter{nullptr};
  // PyObject* next(PyObject* object);
  VirtualFuncType_1_1_SAFE next{nullptr};
  // PyObject* call(PyObject* object, PyList* args, PyDict* kwargs);
  VirtualFuncType_1_3_SAFE call{nullptr};
  // PyObject* len(PyObject* object);
  VirtualFuncType_1_1_SAFE len{nullptr};

  // PyObject* contains(PyObject* object);
  VirtualFuncType_1_2_SAFE_BOOL contains{nullptr};

  // void print(PyObject* object);
  VirtualFuncType_0_1_SAFE print{nullptr};
  // PyObject* repr(PyObject* object);
  VirtualFuncType_1_1_SAFE repr{nullptr};
  // void del_subscr(PyObject* object, PyObject* subscr);
  VirtualFuncType_0_2_SAFE del_subscr{nullptr};

  // 获取实例对象大小，调用该函数绝对不允许触发GC
  // size_t instance_object(PyObject*);
  size_t (*instance_size)(Oop){nullptr};
};
/////////////////虚函数表 定义结束///////////////////////////

class Klass : public Object {
 public:
  Klass() = default;

  Handle<PyString> name() { return Handle<PyString>(name_); }
  void set_name(Handle<PyString> name) { name_ = *name; }

  VirtualTable vtable_;

 private:
  PyString* name_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_H_
