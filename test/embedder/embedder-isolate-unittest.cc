// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <thread>

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
  {
    Isolate::Scope isolate_scope(isolate);
    for (int i = 0; i < 1000; ++i) {
      HandleScope scope(isolate);
      MaybeLocal<Context> maybe_context = Context::New(isolate);
      ASSERT_FALSE(maybe_context.IsEmpty());
      ContextScope context_scope(maybe_context.ToLocalChecked());
    }
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase1Test, ContextScope_NestedLifo) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
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

TEST(EmbedderContractDeathTest, ExplicitApiButNotEnterIsolateShouldDie) {
#if defined(NDEBUG)
  GTEST_SKIP();
#endif
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_DEATH_IF_SUPPORTED(
      {
        // 没有进入该 isolate 而直接调用 api，
        // 会导致触发 isolate != current 断言而直接崩溃。
        HandleScope scope(isolate);
        Local<String> value = String::New(isolate, "contract");
        (void)value;
      },
      "");
  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderContractDeathTest, CrossThreadUseIsolateShouldDie) {
#if defined(NDEBUG)
  GTEST_SKIP();
#endif  // defined(NDEBUG)

  Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    // 在本线程调用 API，不会有任何问题
    Local<String> value = String::New(isolate, "In the correct thread");
    ASSERT_TRUE(!value.IsEmpty());

    // 绝对不允许 isolate 被一个线程占用的同时，
    // 有另外一个线程尝试去占用并使用该 isolate！
    auto worker_func = [&]() {
      Isolate::Scope isolate_scope(isolate);
      HandleScope scope(isolate);
      Local<String> value = String::New(isolate, "cross-thread");
      (void)value;
    };

    ASSERT_DEATH_IF_SUPPORTED(
        {
          std::thread worker(worker_func);
          worker.join();
        },
        "");
  }

  isolate->Dispose();
  Saauso::Dispose();
}

}  // namespace saauso
