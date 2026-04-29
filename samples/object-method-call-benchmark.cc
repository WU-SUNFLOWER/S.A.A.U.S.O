// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"

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
