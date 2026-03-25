// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {

TEST(EmbedderPhase2Test, StringAndIntegerRoundTrip) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<String> text = String::New(isolate, "phase2");
    ASSERT_FALSE(text.IsEmpty());
    Maybe<std::string> text_value = Local<Value>::Cast(text)->ToString();
    ASSERT_FALSE(text_value.IsNothing());
    EXPECT_EQ(text_value.ToChecked(), "phase2");

    Local<Integer> number = Integer::New(isolate, 42);
    ASSERT_FALSE(number.IsEmpty());
    Maybe<int64_t> number_value = Local<Value>::Cast(number)->ToInteger();
    ASSERT_FALSE(number_value.IsNothing());
    EXPECT_EQ(number_value.ToChecked(), 42);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase2Test, Context_Is_Valid_Value) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
    Local<Value> value = Local<Value>::Cast(context);
    EXPECT_FALSE(value->IsString());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase2Test, HandleScope_Prevents_Memory_Leak) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);
  {
    Isolate::Scope isolate_scope(isolate);
    const size_t baseline = isolate->ValueRegistrySizeForTesting();
    for (int i = 0; i < 10000; ++i) {
      HandleScope scope(isolate);
      Local<String> text = String::New(isolate, "leak-check");
      ASSERT_FALSE(text.IsEmpty());
    }
    EXPECT_EQ(isolate->ValueRegistrySizeForTesting(), baseline);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderGCTest, LocalSurvivesMovingGC) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<String> source = String::New(isolate, "gc-safe");
    ASSERT_FALSE(source.IsEmpty());
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
    MaybeLocal<Script> maybe_gc_script =
        Script::Compile(isolate, String::New(isolate, "sysgc()\n"));
    ASSERT_FALSE(maybe_gc_script.IsEmpty());
    ASSERT_FALSE(maybe_gc_script.ToLocalChecked()->Run(context).IsEmpty());
    Maybe<std::string> result = Local<Value>::Cast(source)->ToString();
    ASSERT_FALSE(result.IsNothing());
    EXPECT_EQ(result.ToChecked(), "gc-safe");
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase2Test, TryCatch_Nested_Capture) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    TryCatch outer_try_catch(isolate);
#if SAAUSO_ENABLE_CPYTHON_COMPILER
    Local<String> source = String::New(isolate, "return_not_defined_name");
    ASSERT_FALSE(source.IsEmpty());
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    {
      TryCatch inner_try_catch(isolate);
      MaybeLocal<Context> maybe_context = Context::New(isolate);
      ASSERT_FALSE(maybe_context.IsEmpty());
      MaybeLocal<Value> run_result =
          maybe_script.ToLocalChecked()->Run(maybe_context.ToLocalChecked());
      EXPECT_TRUE(run_result.IsEmpty());
      EXPECT_TRUE(inner_try_catch.HasCaught());
      EXPECT_FALSE(inner_try_catch.Exception().IsEmpty());
    }
#else
    Local<String> source = String::New(isolate, "print('nested')");
    ASSERT_FALSE(source.IsEmpty());
    {
      TryCatch inner_try_catch(isolate);
      MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
      EXPECT_TRUE(maybe_script.IsEmpty());
      EXPECT_TRUE(inner_try_catch.HasCaught());
      EXPECT_FALSE(inner_try_catch.Exception().IsEmpty());
    }
#endif
    EXPECT_FALSE(outer_try_catch.HasCaught());
    EXPECT_TRUE(outer_try_catch.Exception().IsEmpty());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderContractDeathTest, ExplicitIsolateMismatchShouldDie) {
#if defined(NDEBUG)
  GTEST_SKIP();
#endif
  Saauso::Initialize();

  Isolate* isolate_a = Isolate::New();
  Isolate* isolate_b = Isolate::New();
  ASSERT_NE(isolate_a, nullptr);
  ASSERT_NE(isolate_b, nullptr);

  Isolate::Scope isolate_scope(isolate_a);
  HandleScope scope(isolate_a);

  Local<String> source = String::New(isolate_a, "x=1");
  ASSERT_FALSE(source.IsEmpty());

  // 当前 Isolate::Current 指向 isolate_a，严禁直接使用 isolate_b
  ASSERT_DEATH_IF_SUPPORTED(
      {
        Local<String> source2 = String::New(isolate_b, "x=1");
        (void)source2;
      },
      "");
}

TEST(EmbedderContractDeathTest, ExplicitNestedIsolate) {
#if defined(NDEBUG)
  GTEST_SKIP();
#endif
  Saauso::Initialize();

  Isolate* isolate_a = Isolate::New();
  Isolate* isolate_b = Isolate::New();
  ASSERT_NE(isolate_a, nullptr);
  ASSERT_NE(isolate_b, nullptr);

  Isolate::Scope isolate_a_scope(isolate_a);
  HandleScope scope_a(isolate_a);

  Local<String> source = String::New(isolate_a, "x=1");
  ASSERT_FALSE(source.IsEmpty());

  {
    Isolate::Scope isolate_b_scope(isolate_b);
    HandleScope scope_b(isolate_b);
    Local<String> source2 = String::New(isolate_b, "x=2");
    ASSERT_FALSE(source2.IsEmpty());
  }

  Local<String> source3 = String::New(isolate_a, "x=3");
  ASSERT_FALSE(source3.IsEmpty());
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase2Test, ScriptRunSucceedsWithFrontendCompiler) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<String> source = String::New(isolate, "x = 21\nx = x * 2");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());

    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(
        Context::New(isolate).ToLocalChecked());
    EXPECT_FALSE(run_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());

    Local<Value> result;
    ASSERT_TRUE(run_result.ToLocal(&result));
    EXPECT_TRUE(result->ToInteger().IsNothing());
    EXPECT_TRUE(result->ToString().IsNothing());
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#else
TEST(EmbedderPhase2Test,
     TryCatchCapturesCompileFailureWithoutFrontendCompiler) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<String> source = String::New(isolate, "print('phase2')");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());
    EXPECT_TRUE(maybe_script.IsEmpty());

    try_catch.Reset();
    EXPECT_FALSE(try_catch.HasCaught());
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#endif

}  // namespace saauso
