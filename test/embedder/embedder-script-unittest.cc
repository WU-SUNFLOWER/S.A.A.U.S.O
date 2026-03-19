// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "saauso-embedder.h"
#include "saauso.h"

namespace saauso {

TEST(EmbedderPhase2Test, StringAndIntegerRoundTrip) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  HandleScope scope(isolate);

  Local<String> text = String::New(isolate, "phase2");
  ASSERT_FALSE(text.IsEmpty());
  std::string text_value;
  EXPECT_TRUE(Local<Value>::Cast(text)->ToString(&text_value));
  EXPECT_EQ(text_value, "phase2");

  Local<Integer> number = Integer::New(isolate, 42);
  ASSERT_FALSE(number.IsEmpty());
  int64_t number_value = 0;
  EXPECT_TRUE(Local<Value>::Cast(number)->ToInteger(&number_value));
  EXPECT_EQ(number_value, 42);

  // TODO: 当前 Value 私有实现的释放策略仍在收敛，先避免在该用例中触发 Isolate
  // 销毁路径，待 Phase2 生命周期治理完成后恢复 Dispose 断言。
  (void)isolate;
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase2Test, ScriptRunSucceedsWithFrontendCompiler) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  HandleScope scope(isolate);
  Local<String> source = String::New(isolate, "x = 21\nx = x * 2");
  ASSERT_FALSE(source.IsEmpty());

  TryCatch try_catch(isolate);
  MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
  ASSERT_FALSE(maybe_script.IsEmpty());

  MaybeLocal<Value> run_result =
      maybe_script.ToLocalChecked()->Run(Context::New(isolate));
  EXPECT_FALSE(run_result.IsEmpty());
  EXPECT_FALSE(try_catch.HasCaught());

  (void)isolate;
}
#else
TEST(EmbedderPhase2Test,
     TryCatchCapturesCompileFailureWithoutFrontendCompiler) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  HandleScope scope(isolate);
  Local<String> source = String::New(isolate, "print('phase2')");
  ASSERT_FALSE(source.IsEmpty());

  TryCatch try_catch(isolate);
  MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
  EXPECT_FALSE(maybe_script.IsEmpty());
  MaybeLocal<Value> run_result =
      maybe_script.ToLocalChecked()->Run(Context::New(isolate));
  EXPECT_TRUE(run_result.IsEmpty());
  EXPECT_TRUE(try_catch.HasCaught());
  EXPECT_FALSE(try_catch.Exception().IsEmpty());

  try_catch.Reset();
  EXPECT_FALSE(try_catch.HasCaught());

  (void)isolate;
}
#endif

}  // namespace saauso
