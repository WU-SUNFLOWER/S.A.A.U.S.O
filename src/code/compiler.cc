// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/compiler.h"

#include <span>
#include <string_view>

#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"

namespace saauso::internal {
namespace {}  // namespace

Handle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                           Handle<PyString> source,
                                           std::string_view filename) {
  if (source.is_null()) {
    return Handle<PyFunction>::null();
  }
  return CompileSource(isolate, source->buffer(),
                       static_cast<size_t>(source->length()), filename);
}

Handle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                           std::string_view source,
                                           std::string_view filename) {
  return CompileSource(isolate, source.data(), source.size(), filename);
}

Handle<PyFunction> Compiler::CompileSource(Isolate* isolate,
                                           const char* source,
                                           size_t source_size,
                                           std::string_view filename) {
  EscapableHandleScope scope;

  std::vector<uint8_t> pyc = EmbeddedPython312Compiler::CompileToPycBytes(
      std::string_view(source, source_size), filename);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()), isolate);

  Handle<PyCodeObject> code = parser.Parse();
  Handle<PyFunction> func = PyFunction::NewInstance(code);

  return scope.Escape(func);
}

Handle<PyFunction> Compiler::CompilePyc(Isolate* isolate,
                                        std::vector<uint8_t> bytes) {
  EscapableHandleScope scope;
  CPython312PycFileParser parser(
      std::span<const uint8_t>(bytes.data(), bytes.size()), isolate);

  Handle<PyCodeObject> code = parser.Parse();
  Handle<PyFunction> func = PyFunction::NewInstance(code);

  return scope.Escape(func);
}

Handle<PyFunction> Compiler::CompilePyc(Isolate* isolate,
                                        const char* filename) {
  EscapableHandleScope scope;
  CPython312PycFileParser parser(filename, isolate);

  Handle<PyCodeObject> code = parser.Parse();
  Handle<PyFunction> func = PyFunction::NewInstance(code);

  return scope.Escape(func);
}

}  // namespace saauso::internal
