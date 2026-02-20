// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <cstdlib>
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
#include "src/runtime/runtime-exceptions.h"

using namespace saauso::internal;

#if SAAUSO_ENABLE_CPYTHON_COMPILER
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
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

int main(int argc, char** argv) {
  int exit_code = 0;

  saauso::Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope;

#if SAAUSO_ENABLE_CPYTHON_COMPILER
    Handle<PyFunction> boilerplate =
        Compiler::CompileSource(isolate, kSourceCode, kFileName);
#else
    if (argc != 2) {
      std::fprintf(stderr, "Usage: vm <module.pyc>\n");
      std::fprintf(stderr,
                   "Note: build with saauso_enable_cpython_compiler=true to "
                   "run source-code demo.\n");
      std::exit(1);
    }
    Handle<PyFunction> boilerplate = Compiler::CompilePyc(isolate, argv[1]);
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

    isolate->interpreter()->Run(boilerplate);
    if (isolate->exception_state()->HasPendingException()) {
      Handle<PyString> formatted = Runtime_FormatPendingExceptionForStderr();
      std::fprintf(stderr, "%.*s\n", static_cast<int>(formatted->length()),
                   formatted->buffer());
      isolate->exception_state()->Clear();
      exit_code = 1;
    }
  }

  Isolate::Dispose(isolate);
  saauso::Saauso::Dispose();
  return exit_code;
}
