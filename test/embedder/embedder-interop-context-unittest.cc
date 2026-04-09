// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {
namespace {

int64_t g_last_status = -1;

void HostSetStatus(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info[0]->ToInteger();
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_SetStatus expects an integer argument");
    return;
  }
  g_last_status = code.ToChecked();
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code.ToChecked())));
}

TEST(EmbedderPhase3Test, Context_Get_Miss_Test) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);

    HandleScope scope(isolate);

    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Value> miss = context->Get(String::New(isolate, "missing_key"));
    EXPECT_TRUE(miss.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, Context_Global_ObjectBridge) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Object> global = context->Global();
    ASSERT_FALSE(global.IsEmpty());

    EXPECT_TRUE(global
                    ->Set(String::New(isolate, "score"),
                          Local<Value>::Cast(Integer::New(isolate, 88)))
                    .IsJust());
    MaybeLocal<Value> score_value = context->Get(String::New(isolate, "score"));
    ASSERT_FALSE(score_value.IsEmpty());
    Local<Value> score_local;
    ASSERT_TRUE(score_value.ToLocal(&score_local));
    Maybe<int64_t> score = score_local->ToInteger();
    ASSERT_FALSE(score.IsNothing());
    EXPECT_EQ(score.ToChecked(), 88);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, MultiContext_VariableIsolation) {
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

    EXPECT_TRUE(context_a
                    ->Set(String::New(isolate, "score"),
                          Local<Value>::Cast(Integer::New(isolate, 7)))
                    .IsJust());

    MaybeLocal<Value> score_a = context_a->Get(String::New(isolate, "score"));
    ASSERT_FALSE(score_a.IsEmpty());
    Local<Value> score_a_local;
    ASSERT_TRUE(score_a.ToLocal(&score_a_local));
    Maybe<int64_t> score = score_a_local->ToInteger();
    ASSERT_FALSE(score.IsNothing());
    EXPECT_EQ(score.ToChecked(), 7);

    MaybeLocal<Value> score_b = context_b->Get(String::New(isolate, "score"));
    EXPECT_TRUE(score_b.IsEmpty());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, MultiContext_CallbackBindingIsolation) {
  g_last_status = -1;
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

    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context_a
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());

    MaybeLocal<Value> missing =
        context_b->Get(String::New(isolate, "Host_SetStatus"));
    EXPECT_TRUE(missing.IsEmpty());

#if SAAUSO_ENABLE_CPYTHON_COMPILER
    Local<String> source = String::New(isolate, "Host_SetStatus(123)\n");
    ASSERT_FALSE(source.IsEmpty());
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());

    TryCatch ok_try_catch(isolate);
    MaybeLocal<Value> run_a = maybe_script.ToLocalChecked()->Run(context_a);
    ASSERT_FALSE(run_a.IsEmpty());
    EXPECT_FALSE(ok_try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 123);

    TryCatch fail_try_catch(isolate);
    MaybeLocal<Value> run_b = maybe_script.ToLocalChecked()->Run(context_b);
    EXPECT_TRUE(run_b.IsEmpty());
    EXPECT_TRUE(fail_try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 123);
#endif
  }

  isolate->Dispose();
  Saauso::Dispose();
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase4Test, MultiContext_ExceptionIsolation) {
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

    Local<String> bad_source = String::New(isolate, "not_defined_name\n");
    Local<String> good_source = String::New(isolate, "ok = 1\n");
    ASSERT_FALSE(bad_source.IsEmpty());
    ASSERT_FALSE(good_source.IsEmpty());

    MaybeLocal<Script> maybe_bad_script = Script::Compile(isolate, bad_source);
    MaybeLocal<Script> maybe_good_script =
        Script::Compile(isolate, good_source);
    ASSERT_FALSE(maybe_bad_script.IsEmpty());
    ASSERT_FALSE(maybe_good_script.IsEmpty());

    TryCatch bad_try_catch(isolate);
    MaybeLocal<Value> bad_run =
        maybe_bad_script.ToLocalChecked()->Run(context_a);
    EXPECT_TRUE(bad_run.IsEmpty());
    EXPECT_TRUE(bad_try_catch.HasCaught());

    TryCatch good_try_catch(isolate);
    MaybeLocal<Value> good_run =
        maybe_good_script.ToLocalChecked()->Run(context_b);
    EXPECT_FALSE(good_run.IsEmpty());
    EXPECT_FALSE(good_try_catch.HasCaught());
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#endif

}  // namespace
}  // namespace saauso
