// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/compiler.h"

#include <span>
#include <string_view>

#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {
namespace {
constexpr std::string_view kDefaultFileName = "<saauso>";
}  // namespace

Handle<PyCodeObject> Compiler::CompileSource(Isolate* isolate,
                                             Handle<PyString> source) {
  if (source.is_null()) {
    return Handle<PyCodeObject>::null();
  }
  return CompileSource(isolate, source->buffer(),
                       static_cast<size_t>(source->length()), kDefaultFileName);
}

Handle<PyCodeObject> Compiler::CompileSource(Isolate* isolate,
                                             const char* source,
                                             size_t source_size) {
  return CompileSource(isolate, source, source_size, kDefaultFileName);
}

Handle<PyCodeObject> Compiler::CompileSource(Isolate* isolate,
                                             std::string_view source,
                                             std::string_view filename) {
  return CompileSource(isolate, source.data(), source.size(), filename);
}

Handle<PyCodeObject> Compiler::CompileSource(Isolate* isolate,
                                             const char* source,
                                             size_t source_size,
                                             std::string_view filename) {
  EscapableHandleScope scope;

  std::vector<uint8_t> pyc = EmbeddedPython312Compiler::CompileToPycBytes(
      std::string_view(source, source_size), filename);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()), isolate);
  Handle<PyCodeObject> code = parser.Parse();
  return scope.Escape(code);
}

Handle<PyCodeObject> Compiler::CompilePyc(Isolate* isolate,
                                          std::vector<uint8_t> bytes) {
  EscapableHandleScope scope;
  CPython312PycFileParser parser(
      std::span<const uint8_t>(bytes.data(), bytes.size()), isolate);
  Handle<PyCodeObject> code = parser.Parse();
  return scope.Escape(code);
}

Handle<PyCodeObject> Compiler::CompilePyc(Isolate* isolate,
                                          const char* filename) {
  EscapableHandleScope scope;
  CPython312PycFileParser parser(filename, isolate);
  Handle<PyCodeObject> code = parser.Parse();
  return scope.Escape(code);
}

}  // namespace saauso::internal
