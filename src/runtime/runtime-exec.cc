// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <cstdlib>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

// 执行一个 code object，并显式指定其运行环境（locals/globals）。
// 该函数的核心用途是为内建 exec 等路径提供“在指定字典中执行代码”的能力。
Handle<PyObject> Runtime_ExecutePyCodeObject(Handle<PyCodeObject> code,
                                             Handle<PyDict> locals,
                                             Handle<PyDict> globals) {
  EscapableHandleScope scope;

  // 当前异常体系尚未完善，统一采用 fail-fast：stderr + exit(1)。
  if (code.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: code object must not be null\n");
    std::exit(1);
  }
  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: locals and globals must not be null\n");
    std::exit(1);
  }

  // 对齐 CPython：若 globals 未提供 __builtins__，则自动注入当前解释器的
  // builtins。 否则源码中的内建符号（如 print/len/...）将无法解析。
  if (globals->Get(ST(builtins)).is_null()) {
    PyDict::Put(globals, ST(builtins),
                Isolate::Current()->interpreter()->builtins());
  }

  // 将 code object 包装为一个可调用的 PyFunction，随后以“绑定 locals 作为
  // frame.locals” 的方式驱动解释器执行。
  Handle<PyFunction> func = PyFunction::NewInstance(code);
  func->set_func_globals(globals);

  Handle<PyObject> result = Isolate::Current()->interpreter()->CallPython(
      func, Handle<PyObject>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), locals);
  return scope.Escape(result);
}

// PyString 重载仅用于做薄封装，最终走 string_view 版本统一实现。
Handle<PyObject> Runtime_ExecutePythonSourceCode(Handle<PyString> source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals,
                                                 std::string_view filename) {
  if (source.is_null()) {
    return Handle<PyObject>::null();
  }
  return Runtime_ExecutePythonSourceCode(
      std::string_view(source->buffer(), static_cast<size_t>(source->length())),
      locals, globals, filename);
}

// 编译并执行一段 Python 源码，并显式指定其运行环境（locals/globals）。
Handle<PyObject> Runtime_ExecutePythonSourceCode(std::string_view source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals,
                                                 std::string_view filename) {
  EscapableHandleScope scope;

  // 当前异常体系尚未完善，统一采用 fail-fast：stderr + exit(1)。
  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: locals and globals must not be null\n");
    std::exit(1);
  }

  // 对齐 CPython：若 globals 未提供 __builtins__，则自动注入当前解释器的
  // builtins。
  if (globals->Get(ST(builtins)).is_null()) {
    PyDict::Put(globals, ST(builtins),
                Isolate::Current()->interpreter()->builtins());
  }

  // 将源码编译为模块级 boilerplate function，然后在指定字典环境中执行它。
  Handle<PyFunction> func =
      Compiler::CompileSource(Isolate::Current(), source, filename);
  func->set_func_globals(globals);

  Handle<PyObject> result = Isolate::Current()->interpreter()->CallPython(
      func, Handle<PyObject>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), locals);
  return scope.Escape(result);
}

}  // namespace saauso::internal
