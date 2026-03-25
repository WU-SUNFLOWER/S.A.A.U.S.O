// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-exec.h"

#include "src/code/compiler.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

// 对齐 CPython：若 globals 未提供 __builtins__，则自动注入当前解释器的
// builtins。
MaybeHandle<PyObject> InjectDefaultBuiltinsToGlobalsIfNeeded(
    Isolate* isolate,
    Handle<PyDict> globals) {
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             globals->ContainsKey(ST(builtins)));

  if (!found) {
    Handle<PyDict> builtins = handle(isolate->builtins());
    RETURN_ON_EXCEPTION_VALUE(isolate,
                              PyDict::Put(globals, ST(builtins), builtins),
                              kNullMaybeHandle);
  }

  return handle(isolate->py_none_object());
}

}  // namespace

// 执行一个 code object，并显式指定其运行环境（locals/globals）。
// 该函数的核心用途是为内建 exec 等路径提供“在指定字典中执行代码”的能力。
MaybeHandle<PyObject> Runtime_ExecutePyCodeObject(Isolate* isolate,
                                                  Handle<PyCodeObject> code,
                                                  Handle<PyDict> locals,
                                                  Handle<PyDict> globals) {
  EscapableHandleScope scope;

  if (code.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "code object must not be null");
    return kNullMaybeHandle;
  }
  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "locals and globals must not be null");
    return kNullMaybeHandle;
  }

  RETURN_ON_EXCEPTION(isolate,
                      InjectDefaultBuiltinsToGlobalsIfNeeded(isolate, globals));

  // 将 code object 包装为一个可调用的 PyFunction，随后以“绑定 locals 作为
  // frame.locals” 的方式驱动解释器执行。
  Handle<PyFunction> func;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, func, isolate->factory()->NewPyFunctionWithCodeObject(code));
  func->set_func_globals(globals);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, func, Handle<PyObject>::null(),
                      Handle<PyTuple>::null(), Handle<PyDict>::null(), locals));

  return scope.Escape(result);
}

// PyString 重载仅用于做薄封装，最终走 string_view 版本统一实现。
MaybeHandle<PyObject> Runtime_ExecutePythonSourceCode(
    Isolate* isolate,
    Handle<PyString> source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename) {
  assert(!source.is_null());
  return Runtime_ExecutePythonSourceCode(
      isolate,
      std::string_view(source->buffer(), static_cast<size_t>(source->length())),
      locals, globals, filename);
}

// 编译并执行一段 Python 源码，并显式指定其运行环境（locals/globals）。
MaybeHandle<PyObject> Runtime_ExecutePythonSourceCode(
    Isolate* isolate,
    std::string_view source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename) {
  EscapableHandleScope scope;

  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "locals and globals must not be null");
    return kNullMaybeHandle;
  }

#if !SAAUSO_ENABLE_CPYTHON_COMPILER
  Runtime_ThrowError(
      ExceptionType::kRuntimeError,
      "executing Python source requires embedded CPython compiler; build with "
      "saauso_enable_cpython_compiler=true");
  return kNullMaybeHandle;
#else  // !SAAUSO_ENABLE_CPYTHON_COMPILER

  RETURN_ON_EXCEPTION(isolate,
                      InjectDefaultBuiltinsToGlobalsIfNeeded(isolate, globals));

  // 将源码编译为模块级 boilerplate function，然后。
  Handle<PyFunction> boilerplate;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, boilerplate, Compiler::CompileSource(isolate, source, filename));

  boilerplate->set_func_globals(globals);

  Handle<PyObject> result;
  // 在指定字典环境中执行boilerplate
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, boilerplate, Handle<PyObject>::null(),
                      Handle<PyTuple>::null(), Handle<PyDict>::null(), locals));

  return scope.Escape(result);
#endif
}

MaybeHandle<PyObject> Runtime_ExecutePythonPycFile(std::string_view filename,
                                                   Handle<PyDict> locals,
                                                   Handle<PyDict> globals) {
  EscapableHandleScope scope;

  auto* isolate = Isolate::Current();

  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "locals and globals must not be null");
    return kNullMaybeHandle;
  }

  RETURN_ON_EXCEPTION(isolate,
                      InjectDefaultBuiltinsToGlobalsIfNeeded(isolate, globals));

  std::string filename_str(filename);
  Handle<PyFunction> boilerplate;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, boilerplate,
      Compiler::CompilePyc(isolate, filename_str.c_str()));

  boilerplate->set_func_globals(globals);

  Handle<PyObject> result;

  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, boilerplate, Handle<PyObject>::null(),
                      Handle<PyTuple>::null(), Handle<PyDict>::null(), locals));

  return scope.Escape(result);
}

}  // namespace saauso::internal
