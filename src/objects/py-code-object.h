// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_CODE_OBJECT_H_
#define SAAUSO_OBJECTS_CODE_OBJECT_H_

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class Isolate;

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

  static Handle<PyCodeObject> New(Isolate* isolate,
                                  int arg_count,
                                  int posonly_arg_count,
                                  int kwonly_arg_count,
                                  int stack_size,
                                  int flags,
                                  Handle<PyString> bytecodes,
                                  Handle<PyTuple> consts,
                                  Handle<PyTuple> names,
                                  Handle<PyTuple> localsplusnames,
                                  Handle<PyString> localspluskinds,
                                  Handle<PyString> file_name,
                                  Handle<PyString> co_name,
                                  Handle<PyString> qual_name,
                                  int line_no,
                                  Handle<PyString> line_table,
                                  Handle<PyString> exception_table);

  static Tagged<PyCodeObject> cast(Tagged<PyObject> object);

  Handle<PyString> bytecodes() const;

  Handle<PyTuple> consts() const;

  Handle<PyString> co_name() const;
  Handle<PyString> file_name() const;

  int flags() const { return flags_; }
  int stack_size() const { return stack_size_; }
  int arg_count() const { return arg_count_; }

  // 特别说明：
  // 在Python3.11+中，varnames、cellvars、freevars可能有重叠。
  // 比如下面这个例子。
  // ```python
  // def foo(x, y):
  //     def bar():
  //         print(x)
  //         def baz():
  //             print(y)
  //         return baz
  //     return bar
  // ```
  // 其中：
  // - foo的varnames为('x', 'y', 'bar')
  // - foo的cellvars为('x', 'y')
  // - bar的varnames为('baz',)
  // - bar的freevars为('x', 'y')
  // - baz的freevars为('y',)
  // 可见foo的varnames和cellvars中的内容就有重叠。
  //
  // 这也就可以解释为什么在Python3.11中，要将varnames、cellvars、freevars三者整合为localsplusnames，
  // 以及为什么nlocals+ncellvars+nfreevars不一定等于nlocalsplus（因为三者有交集的部分）。
  //
  // 对应到解释器栈帧当中，就是localplus元组。这种设计可以进一步节约空间，还能提升cpu
  // cache的命中率。
  Handle<PyTuple> names() const;
  Handle<PyTuple> var_names() const;
  Handle<PyTuple> cell_vars() const;
  Handle<PyTuple> free_vars() const;

  // 解释器栈帧中实际localsplus的长度
  int nlocalsplus() const { return nlocalsplus_; }
  // var_names的长度
  int nlocals() const { return nlocals_; }
  // cell_vars的长度
  int ncellvars() const { return ncellvars_; }
  // free_vars的长度
  int nfreevars() const { return nfreevars_; }

  Handle<PyTuple> localsplusnames() const;
  Handle<PyString> localspluskinds() const;
  Handle<PyString> exception_table() const;

 private:
  friend class PyCodeObjectKlass;
  friend class FrameObject;
  friend class Factory;

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
