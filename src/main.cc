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
class T(tuple):
  pass

t1 = (1, 2)
t2 = T(t1)
t3 = tuple(t1)

print(t1 == t2 == t3) # True
print(t1 is t2) # False
print(t2 is t3) # False
print(t1 is t3) # True
)";
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

bool CheckAndPrintException(Isolate* isolate) {
  if (!isolate->HasPendingException()) {
    return false;
  }

  Handle<PyString> formatted;
  if (!Runtime_FormatPendingExceptionForStderr().To(&formatted)) {
    std::fprintf(stderr, "Runtime_FormatPendingExceptionForStderr Failed!");
    std::exit(1);
  }

  std::fprintf(stderr, "%s\n", formatted->buffer());
  isolate->exception_state()->Clear();

  return true;
}

MaybeHandle<PyFunction> LoadFunctionBoilerplate(Isolate* isolate) {
#if SAAUSO_ENABLE_CPYTHON_COMPILER
  return Compiler::CompileSource(isolate, kSourceCode, kFileName);
#else
  if (argc != 2) {
    std::fprintf(stderr, "Usage: vm <module.pyc>\n");
    std::fprintf(stderr,
                 "Note: build with saauso_enable_cpython_compiler=true to "
                 "run source-code demo.\n");
    std::exit(1);
  }
  return Compiler::CompilePyc(isolate, argv[1]);
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
}

int main(int argc, char** argv) {
  int exit_code = 0;

  saauso::Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope;

    if (isolate->initialized()) {
      Handle<PyFunction> boilerplate;
      if (LoadFunctionBoilerplate(isolate).To(&boilerplate)) {
        if (isolate->interpreter()->Run(boilerplate).IsNothing()) {
          exit_code = 1;
        }
      } else {
        exit_code = 1;
      }
    }

    if (CheckAndPrintException(isolate)) {
      exit_code = 1;
    }
  }

  Isolate::Dispose(isolate);
  saauso::Saauso::Dispose();
  return exit_code;
}
