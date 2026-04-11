// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {
namespace {

int64_t g_last_status = -1;
std::atomic<int64_t> g_parallel_hits{0};

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

void HostGetStatusWithValidation(FunctionCallbackInfo& info) {
  Maybe<std::string> key = info[0]->ToString();
  if (key.IsNothing()) {
    info.ThrowRuntimeError("Host_GetStatus expects a string argument");
    return;
  }
  if (key.ToChecked() != "status") {
    info.ThrowRuntimeError("Host_GetStatus key must be 'status'");
    return;
  }
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), g_last_status)));
}

void HostAccumulateParallel(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info[0]->ToInteger();
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_Acc expects an integer argument");
    return;
  }
  g_parallel_hits.fetch_add(code.ToChecked());
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code.ToChecked())));
}

void HostInnerPlusOne(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info[0]->ToInteger();
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_InnerPlusOne expects an integer argument");
    return;
  }
  info.SetReturnValue(Local<Value>::Cast(
      Integer::New(info.GetIsolate(), code.ToChecked() + 1)));
}

void HostOuterReentrant(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info[0]->ToInteger();
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_OuterReentrant expects an integer argument");
    return;
  }
  Isolate* isolate = info.GetIsolate();

  Local<Function> inner =
      Function::New(isolate, &HostInnerPlusOne, "Host_InnerPlusOne");
  if (inner.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_OuterReentrant failed to create inner callback");
    return;
  }

  Local<Context> context = Context::New(isolate);
  if (context.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_OuterReentrant failed to create execution context");
    return;
  }

  Local<Value> argv[1] = {
      Local<Value>::Cast(Integer::New(isolate, code.ToChecked()))};
  MaybeLocal<Value> result = inner->Call(context, Local<Value>(), 1, argv);

  Local<Value> result_local;
  if (result.IsEmpty() || !result.ToLocal(&result_local)) {
    info.ThrowRuntimeError("Host_OuterReentrant failed to call inner callback");
    return;
  }

  Maybe<int64_t> inner_value = result_local->ToInteger();
  if (inner_value.IsNothing()) {
    info.ThrowRuntimeError("Host_OuterReentrant failed to call inner callback");
    return;
  }
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(isolate, inner_value.ToChecked() + 1)));
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
void HostGetConfig(FunctionCallbackInfo& info) {
  std::string key;
  if (info.Length() > 0) {
    MaybeLocal<Value> arg0 = info[0];
    Local<Value> value;
    if (arg0.ToLocal(&value)) {
      Maybe<std::string> maybe_key = value->ToString();
      if (maybe_key.IsJust()) {
        key = maybe_key.ToChecked();
      }
    }
  }
  const std::string value = key == "mode" ? "release" : "unknown";
  info.SetReturnValue(
      Local<Value>::Cast(String::New(info.GetIsolate(), value)));
}

void HostThrowViaIsolate(FunctionCallbackInfo& info) {
  Isolate* isolate = info.GetIsolate();
  MaybeLocal<Value> error =
      Exception::RuntimeError(String::New(isolate, "host-isolate-throw"));
  if (error.IsEmpty()) {
    return;
  }
  isolate->ThrowException(error.ToLocalChecked());
}

void HostCallGetterWithBadArg(FunctionCallbackInfo& info) {
  Isolate* isolate = info.GetIsolate();
  Local<Function> getter =
      Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
  if (getter.IsEmpty()) {
    info.ThrowRuntimeError("Host_CallGetterWithBadArg failed to create getter");
    return;
  }
  Local<Context> context = Context::New(isolate);
  if (context.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_CallGetterWithBadArg failed to create execution context");
    return;
  }
  Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 7))};
  MaybeLocal<Value> result = getter->Call(context, Local<Value>(), 1, argv);
  if (!result.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_CallGetterWithBadArg expected inner call to fail");
  }
}
#endif

}  // namespace

TEST(EmbedderPhase3Test, FunctionCall_DirectRoundTrip) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);

    HandleScope scope(isolate);

    MaybeLocal<Function> maybe_func =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(maybe_func.IsEmpty());
    Local<Function> func = maybe_func.ToLocalChecked();
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 301))};
    MaybeLocal<Value> result = func->Call(context, Local<Value>(), 1, argv);
    ASSERT_FALSE(result.IsEmpty());
    Local<Value> result_value;
    ASSERT_TRUE(result.ToLocal(&result_value));
    Maybe<int64_t> code = result_value->ToInteger();
    ASSERT_FALSE(code.IsNothing());
    EXPECT_EQ(code.ToChecked(), 301);
    EXPECT_EQ(g_last_status, 301);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, FunctionCallbackInfo_ArgumentValidation) {
  g_last_status = 42;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);

    HandleScope scope(isolate);

    MaybeLocal<Function> maybe_getter =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(maybe_getter.IsEmpty());
    Local<Function> getter = maybe_getter.ToLocalChecked();
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Value> bad_argv[1] = {Local<Value>::Cast(Integer::New(isolate, 7))};
    TryCatch try_catch(isolate);
    MaybeLocal<Value> bad_result =
        getter->Call(context, Local<Value>(), 1, bad_argv);
    EXPECT_TRUE(bad_result.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());

    try_catch.Reset();
    Local<Value> good_argv[1] = {
        Local<Value>::Cast(String::New(isolate, "status"))};
    MaybeLocal<Value> good_result =
        getter->Call(context, Local<Value>(), 1, good_argv);
    ASSERT_FALSE(good_result.IsEmpty());
    Local<Value> value;
    ASSERT_TRUE(good_result.ToLocal(&value));
    Maybe<int64_t> out = value->ToInteger();
    ASSERT_FALSE(out.IsNothing());
    EXPECT_EQ(out.ToChecked(), 42);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, FunctionCall_FailureWithoutTryCatchDoesNotPoisonNextCall) {
  g_last_status = 42;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Function> getter =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(getter.IsEmpty());
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Value> bad_argv[1] = {Local<Value>::Cast(Integer::New(isolate, 7))};
    MaybeLocal<Value> bad_result =
        getter->Call(context, Local<Value>(), 1, bad_argv);
    EXPECT_TRUE(bad_result.IsEmpty());

    Local<Value> good_argv[1] = {
        Local<Value>::Cast(String::New(isolate, "status"))};
    MaybeLocal<Value> good_result =
        getter->Call(context, Local<Value>(), 1, good_argv);
    ASSERT_FALSE(good_result.IsEmpty());
    Local<Value> value;
    ASSERT_TRUE(good_result.ToLocal(&value));
    Maybe<int64_t> out = value->ToInteger();
    ASSERT_FALSE(out.IsNothing());
    EXPECT_EQ(out.ToChecked(), 42);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, MultiIsolate_ParallelCallbackRegression) {
  g_parallel_hits.store(0);
  Saauso::Initialize();
  constexpr int kThreadCount = 4;
  constexpr int kLoops = 200;
  std::vector<std::thread> workers;
  workers.reserve(kThreadCount);
  for (int t = 0; t < kThreadCount; ++t) {
    workers.emplace_back([=]() {
      Isolate* isolate = Isolate::New();
      EXPECT_NE(isolate, nullptr);
      if (isolate == nullptr) {
        return;
      }
      {
        Isolate::Scope isolate_scope(isolate);
        HandleScope scope(isolate);

        Local<Function> func =
            Function::New(isolate, &HostAccumulateParallel, "Host_Acc");
        EXPECT_FALSE(func.IsEmpty());

        Local<Context> context = Context::New(isolate);
        EXPECT_FALSE(context.IsEmpty());
        for (int i = 0; i < kLoops; ++i) {
          Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 1))};
          MaybeLocal<Value> result =
              func->Call(context, Local<Value>(), 1, argv);
          EXPECT_FALSE(result.IsEmpty());
        }
      }
      isolate->Dispose();
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }
  EXPECT_EQ(g_parallel_hits.load(), kThreadCount * kLoops);
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, FunctionCall_Reentrant_Safe) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Function> outer =
        Function::New(isolate, &HostOuterReentrant, "Host_OuterReentrant");
    ASSERT_FALSE(outer.IsEmpty());

    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 10))};
    MaybeLocal<Value> result = outer->Call(context, Local<Value>(), 1, argv);
    ASSERT_FALSE(result.IsEmpty());
    Local<Value> value;
    ASSERT_TRUE(result.ToLocal(&value));
    Maybe<int64_t> out = value->ToInteger();
    ASSERT_FALSE(out.IsNothing());
    EXPECT_EQ(out.ToChecked(), 12);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase3Test, HostMock_PythonToCppRoundTrip) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_get_config =
        Function::New(isolate, &HostGetConfig, "Host_GetConfig");
    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");

    ASSERT_FALSE(host_get_config.IsEmpty());
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_GetConfig"),
                          Local<Value>::Cast(host_get_config))
                    .IsJust());
    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());

    Local<String> source = String::New(isolate,
                                       "cfg = Host_GetConfig('mode')\n"
                                       "if cfg == 'release':\n"
                                       "    Host_SetStatus(200)\n"
                                       "else:\n"
                                       "    Host_SetStatus(500)\n");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    ASSERT_FALSE(run_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 200);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, CallbackThrow_PythonToTryCatch) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_get_status =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(host_get_status.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_GetStatus"),
                          Local<Value>::Cast(host_get_status))
                    .IsJust());

    Local<String> source = String::New(isolate, "Host_GetStatus(123)\n");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    EXPECT_TRUE(run_result.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test,
     IsolateThrowException_PythonExceptWinsOverOuterTryCatch) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_throw =
        Function::New(isolate, &HostThrowViaIsolate, "Host_ThrowViaIsolate");
    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_throw.IsEmpty());
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_ThrowViaIsolate"),
                          Local<Value>::Cast(host_throw))
                    .IsJust());
    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());

    Local<String> source = String::New(isolate,
                                       "try:\n"
                                       "    Host_ThrowViaIsolate()\n"
                                       "except:\n"
                                       "    Host_SetStatus(777)\n");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    ASSERT_FALSE(run_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 777);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test,
     NestedApiBoundaryWithoutTryCatch_StillPropagatesToPythonExcept) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_inner_fail = Function::New(
        isolate, &HostCallGetterWithBadArg, "Host_CallGetterWithBadArg");
    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_inner_fail.IsEmpty());
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_CallGetterWithBadArg"),
                          Local<Value>::Cast(host_inner_fail))
                    .IsJust());
    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());

    Local<String> source = String::New(isolate,
                                       "try:\n"
                                       "    Host_CallGetterWithBadArg()\n"
                                       "except:\n"
                                       "    Host_SetStatus(888)\n");
    ASSERT_FALSE(source.IsEmpty());

    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    ASSERT_FALSE(run_result.IsEmpty());
    EXPECT_EQ(g_last_status, 888);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, IsolateThrowException_UnhandledFallsBackToTryCatch) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_throw =
        Function::New(isolate, &HostThrowViaIsolate, "Host_ThrowViaIsolate");
    ASSERT_FALSE(host_throw.IsEmpty());
    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_ThrowViaIsolate"),
                          Local<Value>::Cast(host_throw))
                    .IsJust());

    Local<String> source = String::New(isolate, "Host_ThrowViaIsolate()\n");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    EXPECT_TRUE(run_result.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
    Local<Value> exception = try_catch.Exception();
    ASSERT_FALSE(exception.IsEmpty());
    Local<String> message = try_catch.Message();
    ASSERT_FALSE(message.IsEmpty());
    EXPECT_EQ(message->Value(), "host-isolate-throw");
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, Callback_Stress_RemainsCorrectAfterRepeatedCalls) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());

    /////////////////////////////////////////////////////////////////
    // 测试 Part 1
    Local<String> source = String::New(isolate,
                                       "i = 0\n"
                                       "while i < 5000:\n"
                                       "    Host_SetStatus(i)\n"
                                       "    i = i + 1\n");
    ASSERT_FALSE(source.IsEmpty());

    TryCatch try_catch(isolate);

    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());

    // 验证 source 执行后，Host_SetStatus是否的确被调用了 4999 次
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    ASSERT_FALSE(run_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 4999);

    // 验证 source 执行后，i 是否的确变为了 5000
    MaybeLocal<Value> loop_index = context->Get(String::New(isolate, "i"));
    ASSERT_FALSE(loop_index.IsEmpty());
    Local<Value> loop_index_value = loop_index.ToLocalChecked();
    Maybe<int64_t> loop_index_int = loop_index_value->ToInteger();
    ASSERT_FALSE(loop_index_int.IsNothing());
    EXPECT_EQ(loop_index_int.ToChecked(), 5000);

    /////////////////////////////////////////////////////////////////
    // 测试 Part 2
    Local<String> followup_source = String::New(
        isolate, "last = Host_SetStatus(777)\nstatus_after = last\n");
    ASSERT_FALSE(followup_source.IsEmpty());
    MaybeLocal<Script> maybe_followup =
        Script::Compile(isolate, followup_source);
    ASSERT_FALSE(maybe_followup.IsEmpty());
    MaybeLocal<Value> followup_result =
        maybe_followup.ToLocalChecked()->Run(context);

    // 验证脚本执行后，Python 代码是否成功调用 Host_SetStatus Native 函数，并将
    // g_last_status 更新为 777
    ASSERT_FALSE(followup_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
    EXPECT_EQ(g_last_status, 777);

    // 验证脚本执行后，Python 代码是否成功将自己世界的值导出回 C++ 世界的
    // Context
    MaybeLocal<Value> status_after =
        context->Get(String::New(isolate, "status_after"));
    ASSERT_FALSE(status_after.IsEmpty());
    Local<Value> status_after_value = status_after.ToLocalChecked();
    Maybe<int64_t> status_after_int = status_after_value->ToInteger();
    ASSERT_FALSE(status_after_int.IsNothing());
    EXPECT_EQ(status_after_int.ToChecked(), 777);
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

}  // namespace saauso
