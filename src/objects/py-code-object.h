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
  enum Flag {
    kOptimized = 0x0001,    // 我们的虚拟机当中不关心
    kNewLocals = 0x0002,    // 我们的虚拟机当中不关心
    kVarArgs = 0x0004,      // 标记一个函数带有扩展位置参数
    kVarKeywords = 0x0008,  // 标记一个函数带有扩展键值对参数
    kNested = 0x0010,
    kGenerator = 0x0020,
  };

  enum LocalsPlusKind {
    kFastLocalHidden = 0x10,
    kFastLocal = 0x20,
    kFastCell = 0x40,
    kFastFree = 0x80,
  };

  static Handle<PyCodeObject> NewInstance(int arg_count,
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
  Handle<PyTuple> var_names() const;
  Handle<PyTuple> free_vars() const;
  Handle<PyTuple> cell_vars() const;

  Handle<PyTuple> consts() const;

  Handle<PyString> co_name() const;
  Handle<PyString> file_name() const;

  int flags() const { return flags_; }
  int stack_size() const { return stack_size_; }
  int arg_count() const { return arg_count_; }

  int nlocalsplus() const { return nlocalsplus_; }
  int nlocals() const { return nlocals_; }
  int ncellvars() const { return ncellvars_; }
  int nfreevars() const { return nfreevars_; }

 private:
  friend class PyCodeObjectKlass;
  friend class FrameObject;

  int arg_count_;          // 参数个数
  int posonly_arg_count_;  // 位置参数个数
  int kwonly_arg_count_;   // 键参数个数
  int stack_size_;         // 操作数栈深度的最大值
  int flags_;

  // PyString*
  Tagged<PyObject> bytecodes_;
  // PyTuple* 常量表
  Tagged<PyObject> consts_;
  // PyTuple*
  Tagged<PyObject> localsplusnames_;
  // PyString* (bytes)
  Tagged<PyObject> localspluskinds_;

  // PyTuple* 符号表（符号的定义不在函数自己内部）
  Tagged<PyObject> names_;
  // PyTuple* 函数局部变量符号表
  Tagged<PyObject> var_names_;
  // PyTuple*
  Tagged<PyObject> free_vars_;
  // PyTuple*
  Tagged<PyObject> cell_vars_;

  int nlocalsplus_;  // localsplus数组总长度
  int nlocals_;      // 局部变量个数
  int ncellvars_;    // cell变量个数
  int nfreevars_;    // 自由变量个数

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
