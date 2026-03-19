// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "saauso-embedder.h"
#include "saauso.h"

namespace saauso {

TEST(EmbedderPhase1Test, CreateDisposeIsolate) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);
  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase1Test, CreateHandleScopeAndContext) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  for (int i = 0; i < 1000; ++i) {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    ContextScope context_scope(context);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

}  // namespace saauso
