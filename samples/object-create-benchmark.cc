// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "saauso.h"

using namespace saauso;

namespace {

constexpr std::string_view kPythonSource = R"(
import time

class Point:
  def __init__(self, x, y):
    self.x = x
    self.y = y

def main():
  print("before")
  i = 0
  start_time = time.perf_counter()
  while i < 200000:
    point = Point(i, i + 1)
    i += 1
  print("end")
  end_time = time.perf_counter()
  elapsed = end_time - start_time
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

    TryCatch try_catch(isolate);

    Local<Context> context = Context::New(isolate);

    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonSource));
    maybe_script.ToLocalChecked()->Run(context);

    if (try_catch.HasCaught()) {
      Local<String> exception = try_catch.Message();
      std::cout << "exception_msg=" << exception->Value() << std::endl;
      return 1;
    }
  }

  isolate->Dispose();
  Saauso::Dispose();
  return 0;
}
