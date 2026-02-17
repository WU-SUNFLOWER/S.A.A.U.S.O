// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_INTERPRETER_BUILTIN_BOOTSTRAPPER_H_
#define SAAUSO_INTERPRETER_BUILTIN_BOOTSTRAPPER_H_

#include "src/handles/global-handles.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
class PyDict;

// 负责构建并初始化解释器侧的 builtins 字典。
//
// 设计目标：
// - 将 builtins 的安装逻辑从 Interpreter 构造函数中抽离，避免构造函数快速膨胀。
// - 让“新增/删减内建对象”的改动收敛到单点（本类），并便于表驱动化与单测覆盖。
class BuiltinBootstrapper {
 public:
  explicit BuiltinBootstrapper(Isolate* isolate);

  // 创建并返回已完成初始化的 builtins 字典。
  Handle<PyDict> CreateBuiltins();

 private:
  // 注册基础类型的 type object。它们需要在解释器开始执行用户代码前就可用。
  void InstallBuiltinTypes();

  // 注册解释器侧可见的单例对象。
  void InstallOddballs();

  // 注册解释器侧的 native builtins（统一以 PyFunction 包装）。
  void InstallBuiltinFunctions();

  // builtins["builtins"] = builtins，
  // 用于对齐 CPython 的 builtins 自引用行为。
  void InstallBuiltinsSelfReference();

  // 注册内建异常类型（MVP：仅覆盖常用的一小批）。
  void InstallBuiltinExceptionTypes();

  // 注册BaseException和Exception
  void InstallBuiltinBasicExceptionTypes();

  void RegisterSimpleTypeToBuiltins(Handle<PyString> type_name,
                                    Handle<PyObject> type_base);

  Global<PyDict> builtins_;
  bool is_bootstrapped_{false};
  Isolate* isolate_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_INTERPRETER_BUILTIN_BOOTSTRAPPER_H_
