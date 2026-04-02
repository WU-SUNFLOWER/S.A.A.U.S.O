// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>
#include <string_view>
#include <thread>

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {
namespace {

bool RunThreadRoundTrip(Isolate* isolate,
                        int64_t expected_score,
                        std::string_view expected_label) {
  Locker locker(isolate);
  Isolate::Scope isolate_scope(isolate);
  HandleScope scope(isolate);
  TryCatch try_catch(isolate);

  Local<Context> context = Context::New(isolate);
  if (context.IsEmpty()) {
    return false;
  }

  if (!context
           ->Set(String::New(isolate, "score"),
                 Local<Value>::Cast(Integer::New(isolate, expected_score)))
           .IsJust()) {
    return false;
  }
  if (!context
           ->Set(String::New(isolate, "label"),
                 Local<Value>::Cast(String::New(isolate, expected_label)))
           .IsJust()) {
    return false;
  }

  MaybeLocal<Value> score_value = context->Get(String::New(isolate, "score"));
  MaybeLocal<Value> label_value = context->Get(String::New(isolate, "label"));
  if (score_value.IsEmpty() || label_value.IsEmpty() || try_catch.HasCaught()) {
    return false;
  }

  Maybe<int64_t> score = score_value.ToLocalChecked()->ToInteger();
  Maybe<std::string> label = label_value.ToLocalChecked()->ToString();
  if (score.IsNothing() || label.IsNothing()) {
    return false;
  }

  return score.ToChecked() == expected_score &&
         label.ToChecked() == expected_label;
}

}  // namespace

// 验证串行跨线程复用同一个 Isolate 后，公开 Embedder API 仍然可正常工作。
TEST(EmbedderThreadTest, IsolateCanBeReusedSeriallyByMultipleThreads) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  bool thread_a_ok = false;
  std::thread thread_a(
      [&] { thread_a_ok = RunThreadRoundTrip(isolate, 7, "thread-a"); });
  thread_a.join();
  EXPECT_TRUE(thread_a_ok);

  bool thread_b_ok = false;
  std::thread thread_b(
      [&] { thread_b_ok = RunThreadRoundTrip(isolate, 9, "thread-b"); });
  thread_b.join();
  EXPECT_TRUE(thread_b_ok);

  {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "score"),
                          Local<Value>::Cast(Integer::New(isolate, 11)))
                    .IsJust());

    MaybeLocal<Value> score_value = context->Get(String::New(isolate, "score"));
    ASSERT_FALSE(score_value.IsEmpty());
    Maybe<int64_t> score = score_value.ToLocalChecked()->ToInteger();
    ASSERT_FALSE(score.IsNothing());
    EXPECT_EQ(score.ToChecked(), 11);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

}  // namespace saauso
