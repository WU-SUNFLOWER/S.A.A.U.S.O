// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_CODE_OBJECT_H_
#define SAAUSO_OBJECTS_CODE_OBJECT_H_

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyCodeObject : public PyObject {
 public:
  static Handle<PyCodeObject> NewInstance(int arg_count,
                                          int posonly_arg_count,
                                          int kwonly_arg_count,
                                          int nlocals,
                                          int stack_size,
                                          int flags,
                                          Handle<PyString> bytecodes,
                                          Handle<PyList> names,
                                          Handle<PyList> consts,
                                          Handle<PyList> var_names,
                                          Handle<PyList> free_vars,
                                          Handle<PyList> cell_vars,
                                          Handle<PyString> file_name,
                                          Handle<PyString> co_name,
                                          int line_no,
                                          Handle<PyString> no_table);

  static Tagged<PyCodeObject> cast(Tagged<PyObject> object);

  int arg_count_;          // 参数个数
  int posonly_arg_count_;  // 位置参数个数
  int kwonly_arg_count_;   // 键参数个数
  int nlocals_;            // 局部变量个数
  int stack_size_;         // 操作数栈深度的最大值
  int flags_;

  // PyString*
  Tagged<PyObject> bytecodes_;
  // PyList*
  Tagged<PyObject> names_;
  // PyList*
  Tagged<PyObject> consts_;
  // PyList*
  Tagged<PyObject> var_names_;

  // PyList*
  Tagged<PyObject> free_vars_;
  // PyList*
  Tagged<PyObject> cell_vars_;

  // PyString*
  Tagged<PyObject> file_name_;
  // PyString*
  Tagged<PyObject> co_name_;

  int line_no_;
  // PyString*
  Tagged<PyObject> no_table_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_CODE_OBJECT_H_
