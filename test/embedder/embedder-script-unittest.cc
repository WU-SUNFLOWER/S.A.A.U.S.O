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
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase2Test, Context_Is_Valid_Value) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
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

  const size_t baseline = isolate->ValueRegistrySizeForTesting();
  for (int i = 0; i < 10000; ++i) {
    HandleScope scope(isolate);
    Local<String> text = String::New(isolate, "leak-check");
    ASSERT_FALSE(text.IsEmpty());
  }
  EXPECT_EQ(isolate->ValueRegistrySizeForTesting(), baseline);

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderGCTest, LocalSurvivesMovingGC) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<String> source = String::New(isolate, "gc-safe");
    ASSERT_FALSE(source.IsEmpty());
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    MaybeLocal<Script> maybe_gc_script =
        Script::Compile(isolate, String::New(isolate, "sysgc()\n"));
    ASSERT_FALSE(maybe_gc_script.IsEmpty());
    ASSERT_FALSE(maybe_gc_script.ToLocalChecked()->Run(context).IsEmpty());
    std::string result;
    ASSERT_TRUE(Local<Value>::Cast(source)->ToString(&result));
    EXPECT_EQ(result, "gc-safe");
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase2Test, TryCatch_Nested_Capture) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    TryCatch outer_try_catch(isolate);
#if SAAUSO_ENABLE_CPYTHON_COMPILER
    Local<String> source = String::New(isolate, "return_not_defined_name");
    ASSERT_FALSE(source.IsEmpty());
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    {
      TryCatch inner_try_catch(isolate);
      MaybeLocal<Value> run_result =
          maybe_script.ToLocalChecked()->Run(Context::New(isolate));
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

// TODO: Isolate模型完善后开放
// TEST(EmbedderContractDeathTest, ExplicitIsolateMismatchShouldDie) {
//   ASSERT_DEATH_IF_SUPPORTED(
//       {
//         Saauso::Initialize();
//         Isolate* isolate_a = Isolate::New();
//         Isolate* isolate_b = Isolate::New();
//         HandleScope scope(isolate_a);
//         Local<String> source = String::New(isolate_a, "x=1");
//         MaybeLocal<Script> script = Script::Compile(isolate_b, source);
//         (void)script;
//       },
//       "");
// }

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase2Test, ScriptRunSucceedsWithFrontendCompiler) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
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

    Local<Value> result;
    ASSERT_TRUE(run_result.ToLocal(&result));
    int64_t int_value = 0;
    std::string str_value;
    EXPECT_FALSE(result->ToInteger(&int_value));
    EXPECT_FALSE(result->ToString(&str_value));
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
