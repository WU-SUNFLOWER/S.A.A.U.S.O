// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#ifndef SAAUSO_OBJECTS_CELL_H_
#define SAAUSO_OBJECTS_CELL_H_

namespace saauso::internal {

// Cell类是用于实现Python中闭包这一关键特性的基建:
// 1. 它是一个轻量级的容器，用于持有对一个堆上python对象的引用。
// 2. 它本身可以被放在解释器的栈上，可以被LOAD_DEREF、STORE_DEREF
//    等字节码直接操作，因此我们将它视作一个python-object-like对象，
//    于是它会继承自PyObject。
// 3. 它本身对用户的python代码不可见，也不能与其他python对象之间进
//    行任何交互。因此它的klass定义相比正常的python对象会简洁许多。
class Cell : public PyObject {
 public:
  static Tagged<Cell> cast(Tagged<PyObject> object);

  Handle<PyObject> value(Isolate* isolate);
  Tagged<PyObject> value_tagged();
  void set_value(Handle<PyObject> value);
  void set_value(Tagged<PyObject> value);

 private:
  friend class CellKlass;
  friend class Factory;

  Tagged<PyObject> value_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_CELL_H_
