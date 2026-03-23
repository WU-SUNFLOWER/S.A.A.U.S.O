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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    ContextScope context_scope(maybe_context.ToLocalChecked());
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
    MaybeLocal<Context> maybe_context_a = Context::New(isolate);
    MaybeLocal<Context> maybe_context_b = Context::New(isolate);
    ASSERT_FALSE(maybe_context_a.IsEmpty());
    ASSERT_FALSE(maybe_context_b.IsEmpty());
    Local<Context> context_a = maybe_context_a.ToLocalChecked();
    Local<Context> context_b = maybe_context_b.ToLocalChecked();
    {
      ContextScope scope_a(context_a);
      context_b->Enter();
      context_b->Exit();
    }
  }

  isolate->Dispose();
  Saauso::Dispose();
}

// TODO: Isolate模型完善后开放
// TEST(EmbedderContractDeathTest, ExplicitApiWithoutCurrentIsolateShouldDie) {
//   ASSERT_DEATH_IF_SUPPORTED(
//       {
//         Saauso::Initialize();
//         Isolate* isolate = Isolate::New();
//         Local<String> value = String::New(isolate, "contract");
//         (void)value;
//       },
//       "");
// }

}  // namespace saauso
