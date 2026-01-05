// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_CODE_OBJECT_H_
#define SAAUSO_OBJECTS_CODE_OBJECT_H_

#include "handles/handles.h"
#include "objects/py-object.h"

namespace saauso::internal {

class PyString;
class PyList;

class CodeObject : public PyObject {
 public:
  int arg_count_;          // 参数个数
  int posonly_arg_count_;  // 位置参数个数
  int kwonly_arg_count_;   // 键参数个数
  int nlocals_;            // 局部变量个数
  int stack_size_;         // 操作数栈深度的最大值
  int flags_;

  PyString* bytecodes_;
  PyList* names_;
  PyList* consts_;
  PyList* var_names_;

  PyList* free_vars_;
  PyList* cell_vars_;

  PyString* file_name_;
  PyString* co_name_;

  int line_no_;
  PyString* no_table_;

  static Handle<CodeObject> NewInstance(int arg_count,
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
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_CODE_OBJECT_H_
