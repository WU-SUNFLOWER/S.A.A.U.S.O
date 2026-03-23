// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso.h"

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) {
    return 1;
  }

  {
    saauso::Isolate::Scope isolate_scope(isolate);
    saauso::HandleScope scope(isolate);

    saauso::Local<saauso::Context> context =
        saauso::Context::New(isolate).ToLocalChecked();

    saauso::ContextScope context_scope(context);

    saauso::TryCatch try_catch(isolate);

    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "print('Hello World')\n"));
    if (maybe_script.IsEmpty() || try_catch.HasCaught()) {
      return 1;
    }

    maybe_script.ToLocalChecked()->Run(context);
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
