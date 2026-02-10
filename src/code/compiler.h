// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_COMPILER_H_
#define SAAUSO_CODE_COMPILER_H_

#include <string_view>
#include <vector>

#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
class PyString;
class PyFunction;

class Compiler : AllStatic {
 public:
  // 传入Python源代码，需要走CPython的编译器前端进行翻译
  static Handle<PyFunction> CompileSource(Isolate* isolate,
                                          Handle<PyString> source);
  static Handle<PyFunction> CompileSource(Isolate* isolate,
                                          const char* source,
                                          size_t source_size);

  static Handle<PyFunction> CompileSource(Isolate* isolate,
                                          std::string_view source,
                                          std::string_view filename);
  static Handle<PyFunction> CompileSource(Isolate* isolate,
                                          const char* source,
                                          size_t source_size,
                                          std::string_view filename);

  // 传入pyc文件路径或二进制流，直接进行PyCodeObject解析
  static Handle<PyFunction> CompilePyc(Isolate* isolate,
                                       std::vector<uint8_t> bytes);
  static Handle<PyFunction> CompilePyc(Isolate* isolate, const char* filename);
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_COMPILER_H_
