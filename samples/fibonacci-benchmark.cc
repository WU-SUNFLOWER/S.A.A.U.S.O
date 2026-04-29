// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"

namespace {

constexpr std::string_view kPythonSource = R"(
import time

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    start_time = time.perf_counter()
    result = fib(32)
    end_time = time.perf_counter()
    elapsed = end_time - start_time
    print(result)
    print(elapsed)

main()
)";

}  // namespace

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();

  {
    saauso::Isolate::Scope isolate_scope(isolate);
    saauso::HandleScope scope(isolate);

    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, kPythonSource));
    maybe_script.ToLocalChecked()->Run(context);
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
