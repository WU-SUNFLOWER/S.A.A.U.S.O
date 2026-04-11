// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_EXECUTION_EXCEPTION_STATE_H_
#define SAAUSO_EXECUTION_EXCEPTION_STATE_H_

#include "src/handles/handles.h"
#include "src/handles/tagged.h"

namespace saauso::internal {

class ObjectVisitor;

class PyObject;
class PyBaseException;

// 解释器执行期间的“传播态异常状态”（error indicator）。
//
// 设计要点：
// - 该状态属于 Isolate（而不是 Interpreter），便于让 runtime/builtins 在不依赖
//   interpreter 实现细节的情况下统一抛错。
// - 当 pending_exception_ 非空时，解释器将在下一次取指调度前进入异常展开逻辑。
// - pending_exception_pc/origin_pc 的单位为 byte offset，与现有 exception table
//   查表逻辑保持一致。
class ExceptionState final {
 public:
  static constexpr int kInvalidProgramCounter = -1;

  ExceptionState() = default;

  bool HasPendingException() const { return !pending_exception_.is_null(); }

  Tagged<PyObject> pending_exception_tagged() const {
    return pending_exception_;
  }

  Handle<PyObject> pending_exception(Isolate* isolate) const;

  int pending_exception_pc() const { return pending_exception_pc_; }
  void set_pending_exception_pc(int pc) { pending_exception_pc_ = pc; }

  int pending_exception_origin_pc() const {
    return pending_exception_origin_pc_;
  }
  void set_pending_exception_origin_pc(int pc) {
    pending_exception_origin_pc_ = pc;
  }

  // 设置 pending exception，并重置 pc/origin 为无效值。
  // 该接口仅改变 error indicator，不负责构造异常对象。
  void Throw(Handle<PyBaseException> exception);

  void Clear();

  // GC root 扫描接口：暴露 pending_exception_。
  void Iterate(ObjectVisitor* v);

 private:
  Tagged<PyObject> pending_exception_{kNullAddress};
  int pending_exception_pc_{kInvalidProgramCounter};
  int pending_exception_origin_pc_{kInvalidProgramCounter};
};

}  // namespace saauso::internal

#endif  // SAAUSO_EXECUTION_EXCEPTION_STATE_H_
