// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/compiler.h"

#include <cstdio>
#include <cstdlib>
#include <span>
#include <string_view>

#if SAAUSO_ENABLE_CPYTHON_COMPILER
#include "src/code/cpython312-pyc-compiler.h"
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

#include "src/code/cpython312-pyc-file-parser.h"
#include "src/execution/exception-utils.h"
#include "src/heap/factory.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {
namespace {}  // namespace

MaybeHandle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                                Handle<PyString> source,
                                                std::string_view filename) {
  if (source.is_null()) [[unlikely]] {
    return kNullMaybeHandle;
  }
  return CompileSource(isolate, source->buffer(),
                       static_cast<size_t>(source->length()), filename);
}

MaybeHandle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                                std::string_view source,
                                                std::string_view filename) {
  return CompileSource(isolate, source.data(), source.size(), filename);
}

MaybeHandle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                                const char* source,
                                                size_t source_size,
                                                std::string_view filename) {
  EscapableHandleScope scope(isolate);

#if !SAAUSO_ENABLE_CPYTHON_COMPILER
  Runtime_ThrowError(
      ExceptionType::kRuntimeError,
      "RuntimeError: CompileSource() requires embedded CPython "
      "compiler; build with saauso_enable_cpython_compiler=true\n");
  return kNullMaybeHandle;
#else
  std::vector<uint8_t> pyc = EmbeddedPython312Compiler::CompileToPycBytes(
      std::string_view(source, source_size), filename);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()), isolate);
  Handle<PyCodeObject> code = parser.Parse();

  Handle<PyFunction> func =
      isolate->factory()->NewPyFunctionWithCodeObject(code);

  return scope.Escape(func);
#endif  // !SAAUSO_ENABLE_CPYTHON_COMPILER
}

MaybeHandle<PyFunction> Compiler::CompilePyc(Isolate* isolate,
                                             std::vector<uint8_t> bytes) {
  EscapableHandleScope scope(isolate);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(bytes.data(), bytes.size()), isolate);
  Handle<PyCodeObject> code = parser.Parse();

  Handle<PyFunction> func =
      isolate->factory()->NewPyFunctionWithCodeObject(code);

  return scope.Escape(func);
}

MaybeHandle<PyFunction> Compiler::CompilePyc(Isolate* isolate,
                                             const char* filename) {
  EscapableHandleScope scope(isolate);

  CPython312PycFileParser parser(filename, isolate);
  Handle<PyCodeObject> code = parser.Parse();

  Handle<PyFunction> func =
      isolate->factory()->NewPyFunctionWithCodeObject(code);

  return scope.Escape(func);
}

}  // namespace saauso::internal
