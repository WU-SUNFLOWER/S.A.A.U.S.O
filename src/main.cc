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
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"

using namespace saauso::internal;

constexpr std::string_view kFileName = "test.py";
constexpr std::string_view kSourceCode = R"(
s = "xxyyzz"
print(s.index("yy"))
print(s.index("yy", 1))
print(s.index("", 3))
print(s.index("yy", 0, 4))

l = [1, 2, 1, 3]
print(l.index(1))
print(l.index(1, 1))
print(l.index(1, 1, 3))

t = (1, 2, 1, 3)
print(t.index(1))
print(t.index(1, 1))
print(t.index(1, 1, 3))
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
        std::span<const uint8_t>(pyc.data(), pyc.size()), isolate);
    Handle<PyCodeObject> code = parser.Parse();

    isolate->interpreter()->Run(code);
  }

  Isolate::Dispose(isolate);
  saauso::Saauso::Dispose();
}
