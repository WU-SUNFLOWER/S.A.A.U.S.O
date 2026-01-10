// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs-klass.h"

#include <cstdio>

#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PyBooleanKlass> PyBooleanKlass::instance_(nullptr);
Tagged<PyNoneKlass> PyNoneKlass::instance_(nullptr);

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
Tagged<PyBooleanKlass> PyBooleanKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyBooleanKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;

  // 设置类名
  set_name(PyString::NewInstance("bool"));
}

// static
void PyBooleanKlass::Finalize() {
  instance_ = Tagged<PyBooleanKlass>::Null();
}

// static
void PyBooleanKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf(Handle<PyBoolean>::Cast(self)->value() ? "True" : "False");
}

// static
Tagged<PyBoolean> PyBooleanKlass::Virtual_Equal(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  bool v = Handle<PyBoolean>::Cast(self)->value();

  bool result = false;
  if (IsPySmi(other)) {
    result = PySmi::ToInt(Handle<PySmi>::Cast(other)) == v;
  } else if (IsPyFloat(other)) {
    result = Handle<PyFloat>::Cast(other)->value() == v;
  } else if (IsPyBoolean(other)) {
    result = Handle<PyBoolean>::Cast(other)->value() == v;
  }

  return Universe::ToPyBoolean(result);
}

// static
Tagged<PyBoolean> PyBooleanKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                   Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
Tagged<PyNoneKlass> PyNoneKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyNoneKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;

  // 设置类名
  set_name(PyString::NewInstance("NoneType"));
}

void PyNoneKlass::Finalize() {
  instance_ = Tagged<PyNoneKlass>::Null();
}

// static
void PyNoneKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("None");
}

// static
Tagged<PyBoolean> PyNoneKlass::Virtual_Equal(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(IsPyNone(self));
  return Universe::ToPyBoolean((*self).ptr() == (*other).ptr());
}

// static
Tagged<PyBoolean> PyNoneKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

}  // namespace saauso::internal
