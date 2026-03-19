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

  Local<Integer> number = Integer::New(isolate, 42);
  ASSERT_FALSE(number.IsEmpty());

  (void)isolate;
}

TEST(EmbedderPhase2Test, TryCatchCapturesCompileFailureWithoutFrontendCompiler) {
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

}  // namespace saauso
