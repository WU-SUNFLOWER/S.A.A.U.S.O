// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>
#include <string_view>

#include "include/saauso.h"
#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"

using namespace saauso::internal;

constexpr std::string_view kFileName = "test.py";
constexpr std::string_view kSourceCode = R"(
i = 0
while i < 10:
  if i < 2:
    continue
  print(i)
  i = i + 1
)";

int main() {
  saauso::Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope;

    std::vector<uint8_t> pyc =
        CompilePythonSourceToPycBytes312(kSourceCode, kFileName);

    CPython312PycFileParser parser(
        std::span<const uint8_t>(pyc.data(), pyc.size()));
    Handle<PyCodeObject> code = parser.Parse();

    isolate->interpreter()->Run(code);
  }

  Isolate::Dispose(isolate);
  saauso::Saauso::Dispose();
}
