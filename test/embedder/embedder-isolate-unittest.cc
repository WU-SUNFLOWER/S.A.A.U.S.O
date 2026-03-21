// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

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

TEST(EmbedderPhase1Test, ContextScope_NestedLifo) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context_a = Context::New(isolate);
    Local<Context> context_b = Context::New(isolate);
    ASSERT_FALSE(context_a.IsEmpty());
    ASSERT_FALSE(context_b.IsEmpty());
    {
      ContextScope scope_a(context_a);
      context_b->Enter();
      context_b->Exit();
    }
  }

  isolate->Dispose();
  Saauso::Dispose();
}

}  // namespace saauso
