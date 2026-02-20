// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_INTERPRETER_TRY_CATCH_H_
#define SRC_INTERPRETER_TRY_CATCH_H_

#include "src/interpreter/interpreter.h"

namespace saauso::internal {

// VM 内部的 C++ 侧“异常捕获器”（V8-like 语义）。
//
// 该类型基于解释器的 pending exception 作为 error indicator：
// - 在 scope 内发生的 VM 异常会体现在 Interpreter::HasPendingException() 上；
// - 调用方可以读取异常对象、清理异常状态，或让异常继续向外传播。
//
// 重要约束：
// - 该类型不依赖 C++ 语言级异常（不使用 throw/catch）。
// - 若进入 scope 前已有 pending exception，TryCatch 会在 scope 内被清理且
//   scope 结束时恢复它，避免外层异常状态被误吞掉。
class TryCatch final {
 public:
  explicit TryCatch(Interpreter* interpreter)
      : interpreter_(interpreter),
        saved_pending_exception_(interpreter->isolate_->exception_state()
                                     ->pending_exception_tagged()),
        saved_pending_exception_pc_(
            interpreter->isolate_->exception_state()->pending_exception_pc()),
        saved_pending_exception_origin_pc_(
            interpreter->isolate_->exception_state()
                ->pending_exception_origin_pc()) {
    assert(interpreter_ != nullptr);
  }

  TryCatch(const TryCatch&) = delete;
  TryCatch& operator=(const TryCatch&) = delete;

  ~TryCatch() {
    if (saved_pending_exception_.is_null()) {
      return;
    }
    if (interpreter_->HasPendingException()) {
      return;
    }
    auto* state = interpreter_->isolate_->exception_state();
    state->Throw(saved_pending_exception_);
    state->set_pending_exception_pc(saved_pending_exception_pc_);
    state->set_pending_exception_origin_pc(saved_pending_exception_origin_pc_);
  }

  bool HasCaught() const { return interpreter_->HasPendingException(); }

  Handle<PyObject> Exception() const {
    return interpreter_->pending_exception();
  }

  void Reset() { interpreter_->ClearPendingException(); }

 private:
  Interpreter* interpreter_;
  Tagged<PyObject> saved_pending_exception_{kNullAddress};
  int saved_pending_exception_pc_{-1};
  int saved_pending_exception_origin_pc_{-1};
};

}  // namespace saauso::internal

#endif  // SRC_INTERPRETER_TRY_CATCH_H_
