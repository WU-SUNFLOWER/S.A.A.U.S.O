// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_CODE_OBJECT_H_
#define SAAUSO_OBJECTS_CODE_OBJECT_H_

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyString;
class PyTuple;

class PyCodeObject : public PyObject {
 public:
  static Handle<PyCodeObject> NewInstance(int arg_count,
                                          int posonly_arg_count,
                                          int kwonly_arg_count,
                                          int nlocals,
                                          int stack_size,
                                          int flags,
                                          Handle<PyString> bytecodes,
                                          Handle<PyTuple> names,
                                          Handle<PyTuple> consts,
                                          Handle<PyTuple> var_names,
                                          Handle<PyTuple> free_vars,
                                          Handle<PyTuple> cell_vars,
                                          Handle<PyString> file_name,
                                          Handle<PyString> co_name,
                                          int line_no,
                                          Handle<PyString> no_table);

  static Handle<PyCodeObject> NewInstance312(int arg_count,
                                             int posonly_arg_count,
                                             int kwonly_arg_count,
                                             int stack_size,
                                             int flags,
                                             Handle<PyString> bytecodes,
                                             Handle<PyTuple> consts,
                                             Handle<PyTuple> names,
                                             Handle<PyTuple> localsplusnames,
                                             Handle<PyString> localspluskinds,
                                             Handle<PyTuple> var_names,
                                             Handle<PyTuple> free_vars,
                                             Handle<PyTuple> cell_vars,
                                             Handle<PyString> file_name,
                                             Handle<PyString> co_name,
                                             Handle<PyString> qual_name,
                                             int line_no,
                                             Handle<PyString> line_table,
                                             Handle<PyString> exception_table);

  static Tagged<PyCodeObject> cast(Tagged<PyObject> object);

  Handle<PyString> bytecodes() const;
  Handle<PyTuple> names() const;
  Handle<PyTuple> consts() const;
  Handle<PyString> co_name() const;
  Handle<PyString> file_name() const;

  int flags() const { return flags_; }
  int stack_size() const { return stack_size_; }
  int nlocals() const { return nlocals_; }
  int arg_count() const { return arg_count_; }

 private:
  friend class PyCodeObjectKlass;
  friend class FrameObject;

  int arg_count_;          // 参数个数
  int posonly_arg_count_;  // 位置参数个数
  int kwonly_arg_count_;   // 键参数个数
  int nlocals_;            // 局部变量个数
  int stack_size_;         // 操作数栈深度的最大值
  int flags_;

  // PyString*
  Tagged<PyObject> bytecodes_;
  // PyTuple*
  Tagged<PyObject> names_;
  // PyTuple*
  Tagged<PyObject> consts_;
  // PyTuple*
  Tagged<PyObject> localsplusnames_;
  // PyString* (bytes)
  Tagged<PyObject> localspluskinds_;
  // PyTuple*
  Tagged<PyObject> var_names_;

  // PyTuple*
  Tagged<PyObject> free_vars_;
  // PyTuple*
  Tagged<PyObject> cell_vars_;

  // PyString*
  Tagged<PyObject> file_name_;
  // PyString*
  Tagged<PyObject> co_name_;
  // PyString*
  Tagged<PyObject> qual_name_;

  int line_no_;
  // PyString*
  Tagged<PyObject> line_table_;
  // PyString*
  Tagged<PyObject> exception_table_;
  // PyString*
  Tagged<PyObject> no_table_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_CODE_OBJECT_H_
