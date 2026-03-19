// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso-embedder.h"
#include "saauso.h"

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) {
    return 1;
  }

  {
    saauso::HandleScope scope(isolate);
    auto context = saauso::Context::New(isolate);
    saauso::ContextScope context_scope(context);
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
