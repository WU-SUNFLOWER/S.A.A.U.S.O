// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_INTERPRETER_EXCEPTION_TABLE_H_
#define SAAUSO_INTERPRETER_EXCEPTION_TABLE_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyCodeObject;

struct ExceptionHandlerInfo {
  int stack_depth{0};
  int handler_pc{0};
  // lasti是"Last Instruction"的缩写
  // need_push_lasti用于标记传播错误时，
  // 是否需要将发生异常的那条指令的地址压栈。
  bool need_push_lasti{false};
};

class ExceptionTable final {
 public:
  // TODO: 将该抽象升级为真正的 PyExceptionTable（内部不可见的特殊 Python
  // 类型）。
  // TODO: 让 PyCodeObject 的 exception_table_ 字段指向 PyExceptionTable 实例，
  //       而非当前的 raw bytes(PyString) 数据视图。
  // TODO: 把 LookupHandler 的线性扫描升级为可选的二分查找
  static bool LookupHandler(Handle<PyCodeObject> code_object,
                            int instruction_offset_in_bytes,
                            ExceptionHandlerInfo& out);
};

}  // namespace saauso::internal

#endif  // SAAUSO_INTERPRETER_EXCEPTION_TABLE_H_
