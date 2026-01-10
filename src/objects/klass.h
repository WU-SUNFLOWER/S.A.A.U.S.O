// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_H_
#define SAAUSO_OBJECTS_KLASS_H_

#include <cstddef>

#include "src/handles/handles.h"
#include "src/objects/objects.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class ObjectVisitor;
class PyBoolean;

/////////////////虚函数表 定义开始///////////////////////////

struct VirtualTable {
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
  using VirtualFuncType_1_3_SAFE = OopHandle (*)(OopHandle,
                                                 OopHandle,
                                                 OopHandle);

  using VirtualFuncType_1_1_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle);
  using VirtualFuncType_1_2_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle,
                                                              OopHandle);
  using VirtualFuncType_1_3_SAFE_BOOL = Tagged<PyBoolean> (*)(OopHandle,
                                                              OopHandle,
                                                              OopHandle);

  // Tagged<PyObject> add(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE add{nullptr};
  // Tagged<PyObject> sub(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE sub{nullptr};
  // Tagged<PyObject> mul(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE mul{nullptr};
  // Tagged<PyObject> div(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE div{nullptr};

  // Tagged<PyObject> getattr(Tagged<PyObject> object, Tagged<PyObject> key);
  VirtualFuncType_1_2_SAFE getattr{nullptr};
  // Tagged<PyObject> setattr(Tagged<PyObject> object, Tagged<PyObject> key,
  // Tagged<PyObject> value);
  VirtualFuncType_1_3_SAFE setattr{nullptr};
  // Tagged<PyObject> subscr(Tagged<PyObject> object, Tagged<PyObject> subscr);
  VirtualFuncType_1_2_SAFE subscr{nullptr};
  // void store_subscr(Tagged<PyObject> object, Tagged<PyObject> subscr,
  // Tagged<PyObject> value);
  VirtualFuncType_0_3_SAFE store_subscr{nullptr};

  // Tagged<PyBoolean> greater(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL greater{nullptr};
  // Tagged<PyBoolean> less(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL less{nullptr};
  // Tagged<PyBoolean> equal(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL equal{nullptr};
  // Tagged<PyBoolean> not_equal(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL not_equal{nullptr};
  // Tagged<PyBoolean> ge(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL ge{nullptr};
  // Tagged<PyBoolean> le(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE_BOOL le{nullptr};

  // Tagged<PyObject> mod(Tagged<PyObject> a, Tagged<PyObject> b);
  VirtualFuncType_1_2_SAFE mod{nullptr};

  // Tagged<PyObject> iter(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE iter{nullptr};
  // Tagged<PyObject> next(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE next{nullptr};
  // Tagged<PyObject> call(Tagged<PyObject> object, Tagged<PyList> args, PyDict*
  // kwargs);
  VirtualFuncType_1_3_SAFE call{nullptr};
  // Tagged<PyObject> len(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE len{nullptr};

  // Tagged<PyObject> contains(Tagged<PyObject> object);
  VirtualFuncType_1_2_SAFE_BOOL contains{nullptr};

  // void print(Tagged<PyObject> object);
  VirtualFuncType_0_1_SAFE print{nullptr};
  // Tagged<PyObject> repr(Tagged<PyObject> object);
  VirtualFuncType_1_1_SAFE repr{nullptr};
  // void del_subscr(Tagged<PyObject> object, Tagged<PyObject> subscr);
  VirtualFuncType_0_2_SAFE del_subscr{nullptr};

  // 获取实例对象大小，调用该函数绝对不允许触发GC
  // size_t instance_object(Tagged<PyObject>);
  size_t (*instance_size)(Oop){nullptr};

  void (*iterate)(OopHandle, ObjectVisitor*){nullptr};
};
/////////////////虚函数表 定义结束///////////////////////////

class Klass : public Object {
 public:
  Klass() = default;

  Handle<PyString> name() { return Handle<PyString>(name_); }
  void set_name(Handle<PyString> name) { name_ = *name; }

  VirtualTable vtable_;

 private:
  Tagged<PyString> name_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_H_
