// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "saauso.h"

using namespace saauso;

namespace {

constexpr std::string_view kPythonSource = R"(
import time

class Counter:
    def __init__(self, base):
        self.base = base

    def step(self, value):
        return self.base + value

def build_objects():
    i = 0
    result = []
    while i < 1000:
        result.append(Counter(i))
        i += 1
    return result

def main():
    objects = build_objects()
    outer = 0
    total = 0
    start_time = time.perf_counter()
    while outer < 200:
        inner = 0
        while inner < len(objects):
            total = total + objects[inner].step(inner)
            inner += 1
        outer += 1
    end_time = time.perf_counter()
    elapsed = end_time - start_time
    print(total)
    print(elapsed)

main()
)";

}  // namespace

int main() {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Context> context = Context::New(isolate);
    Maybe<void> set_name_result = context->Global()->Set(
        String::New(isolate, "__name__"), String::New(isolate, "__main__"));
    if (set_name_result.IsNothing()) [[unlikely]] {
      std::cout << "set __name__ failed" << std::endl;
      return 1;
    }

    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonSource));
    maybe_script.ToLocalChecked()->Run(context);
  }

  isolate->Dispose();
  Saauso::Dispose();
  return 0;
}
