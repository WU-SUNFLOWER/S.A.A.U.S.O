// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-primitive.h"
#include "include/saauso-script.h"
#include "src/api/api-exception-support.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/code/compiler.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/object-checkers.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-py-dict.h"

namespace saauso {

MaybeLocal<Script> Script::Compile(Isolate* isolate, Local<String> source) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  if (source.IsEmpty()) {
    return MaybeLocal<Script>();
  }

  i::EscapableHandleScope handle_scope(i_isolate);

#if SAAUSO_ENABLE_CPYTHON_COMPILER
  i::Handle<i::PyString> script_source =
      i::PyString::New(i_isolate, source->Value().c_str());
  i::Handle<i::PyFunction> script;
  if (!i::Compiler::CompileSource(i_isolate, script_source).ToHandle(&script)) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return {};
  }

  i::Handle<i::PyObject> escaped = handle_scope.Escape(script);
  return api::Utils::ToLocal<Script>(escaped);
#else   // SAAUSO_ENABLE_CPYTHON_COMPILER
  i::Runtime_ThrowError(
      i::ExceptionType::kRuntimeError,
      "Script::Compile requires CPython frontend compiler support");
  api::FinalizePendingExceptionAtApiBoundary(i_isolate);
  return {};
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
}

MaybeLocal<Value> Script::Run(Local<Context> context) {
  i::Isolate* i_isolate = api::RequireCurrentIsolate();

  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> script_object = api::Utils::OpenHandle(this);
  assert(!script_object.is_null());
  assert(i::IsPyFunction(script_object, i_isolate));

  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(context);
  assert(!context_object.is_null());
  assert(i::IsPyDict(context_object));

  i::EscapableHandleScope handle_scope(i_isolate);

  // TODO(WU-SUNFLOWER): 明确 __name__ 的注入规则应由 Script::Run、
  // Context::New 还是嵌入方自行负责。
  i::Handle<i::PyDict> globals = i::Handle<i::PyDict>::cast(context_object);

  i::Handle<i::PyFunction> script =
      i::Handle<i::PyFunction>::cast(script_object);

  // 执行 script。
  // 特别说明：对于根栈帧来说，它的局部作用域字典（locals）直接复用 globals，
  // 所以这里调用 Execution::CallScript API 时，这两个参数我们都统一传 globals！
  i::MaybeHandle<i::PyObject> maybe_result =
      i::Execution::CallScript(i_isolate, script, globals, globals);

  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::FinalizePendingExceptionAtApiBoundary(i_isolate);
    return {};
  }

  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return api::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
