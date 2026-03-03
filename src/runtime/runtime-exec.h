// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_EXEC_H_
#define SSAUSO_RUNTIME_RUNTIME_EXEC_H_

#include <string_view>

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;
class PyCodeObject;
class PyDict;
class PyString;

inline constexpr std::string_view kDefaultSourceFilename = "<string>";

// 执行一个 PyCodeObject，并在指定 locals/globals 环境中运行。
// - locals/globals 必须是非空的 dict。
// - 若 globals 中缺少 __builtins__，则自动注入 builtins
// dict，保证内建符号可用。
// - 返回值为 code object 执行完成后的返回值（对 exec 来说通常会被上层忽略）。
MaybeHandle<PyObject> Runtime_ExecutePyCodeObject(Handle<PyCodeObject> code,
                                                  Handle<PyDict> locals,
                                                  Handle<PyDict> globals);

// 编译并执行一段 Python 源码，并在指定 locals/globals 环境中运行。
// - locals/globals 必须是非空的 dict。
// - 若 globals 中缺少 __builtins__，则自动注入 builtins dict。
// - 返回值为源码执行完成后的返回值（对 exec 来说通常会被上层忽略）。
MaybeHandle<PyObject> Runtime_ExecutePythonSourceCode(
    Isolate* isolate,
    Handle<PyString> source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename = kDefaultSourceFilename);

// 编译并执行一段 Python 源码，并在指定 locals/globals 环境中运行。
// 语义与 Runtime_ExecutePythonSourceCode(PyString, ...) 一致。
MaybeHandle<PyObject> Runtime_ExecutePythonSourceCode(
    Isolate* isolate,
    std::string_view source,
    Handle<PyDict> locals,
    Handle<PyDict> globals,
    std::string_view filename = kDefaultSourceFilename);

MaybeHandle<PyObject> Runtime_ExecutePythonPycFile(std::string_view filename,
                                                   Handle<PyDict> locals,
                                                   Handle<PyDict> globals);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_EXEC_H_
