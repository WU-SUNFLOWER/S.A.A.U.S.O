// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>
#include <string_view>

#include "include/saauso.h"
#include "src/code/compiler.h"
#include "src/execution/isolate.h"
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

using namespace saauso::internal;

constexpr std::string_view kFileName = "test.py";
constexpr std::string_view kSourceCode = R"(
class A(object):
    def say(self):
        print("I am A")

class B(A):
    def say(self):
        print("I am B")

class C(A):
    pass

b = B()
c = C()

b.say()    # "I am B"
c.say()    # "I am A"
)";

int main() {
  saauso::Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope;

    Handle<PyCodeObject> code =
        Compiler::CompileSource(isolate, kSourceCode, kFileName);

    isolate->interpreter()->Run(code);
  }

  Isolate::Dispose(isolate);
  saauso::Saauso::Dispose();
}
