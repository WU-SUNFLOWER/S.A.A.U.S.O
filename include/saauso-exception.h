// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_EXCEPTION_H_
#define INCLUDE_SAAUSO_EXCEPTION_H_

#include <memory>

#include "saauso-local-handle.h"

namespace saauso {

namespace api {
class ApiBridgeAccess;
}

namespace internal {
class Isolate;
template <typename T>
class Handle;
class PyObject;
}  // namespace internal

class String;

class Exception {
 public:
  // 构造 TypeError 对象。
  // 传入的 message 参数不能为空，否则在 debug 模式下会触发断言！
  static Local<Value> TypeError(Local<String> message);
  // 构造 RuntimeError 对象；失败返回 Nothing。
  // 传入的 message 参数不能为空，否则在 debug 模式下会触发断言！
  static Local<Value> RuntimeError(Local<String> message);
};

class TryCatch {
 public:
  explicit TryCatch(Isolate* isolate);
  TryCatch(const TryCatch&) = delete;
  TryCatch& operator=(const TryCatch&) = delete;
  ~TryCatch();

  bool HasCaught() const;
  void Reset();
  // 返回当前捕获的异常；未捕获时返回空 Local<Value>。
  Local<Value> Exception() const;
  // 返回当前捕获异常的错误信息；未捕获时返回空 Local<String>。
  // MVP 约定：
  // - 若异常为字符串，直接返回该字符串；
  // - 若异常为 BaseException（含子类），返回其 args 解析后的消息；
  // - 其他异常值返回空 Local<String>。
  Local<String> Message() const;

 private:
  struct Impl;
  friend class api::ApiBridgeAccess;
  void SetException(internal::Handle<internal::PyObject> exception);
  int PythonExecutionDepth() const;

  internal::Isolate* i_isolate_{nullptr};
  TryCatch* previous_{nullptr};
  std::unique_ptr<Impl> impl_;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EXCEPTION_H_
