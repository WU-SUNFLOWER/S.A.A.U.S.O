// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "gtest/gtest.h"
#include "saauso-embedder.h"
#include "saauso.h"

namespace saauso {
namespace {

int64_t g_last_status = -1;

void HostSetStatus(FunctionCallbackInfo& info) {
  int64_t code = 0;
  if (!info.GetIntegerArg(0, &code)) {
    info.ThrowRuntimeError("Host_SetStatus expects an integer argument");
    return;
  }
  g_last_status = code;
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code)));
}

void HostGetStatusWithValidation(FunctionCallbackInfo& info) {
  std::string key;
  if (!info.GetStringArg(0, &key)) {
    info.ThrowRuntimeError("Host_GetStatus expects a string argument");
    return;
  }
  if (key != "status") {
    info.ThrowRuntimeError("Host_GetStatus key must be 'status'");
    return;
  }
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), g_last_status)));
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
void HostGetConfig(FunctionCallbackInfo& info) {
  std::string key;
  if (info.Length() > 0) {
    info[0]->ToString(&key);
  }
  const std::string value = key == "mode" ? "release" : "unknown";
  info.SetReturnValue(
      Local<Value>::Cast(String::New(info.GetIsolate(), value)));
}
#endif

}  // namespace

TEST(EmbedderPhase3Test, FunctionCall_DirectRoundTrip) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Function> func =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(func.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 301))};
    MaybeLocal<Value> result =
        func->Call(Context::New(isolate), Local<Value>(), 1, argv);
    ASSERT_FALSE(result.IsEmpty());
    Local<Value> result_value;
    ASSERT_TRUE(result.ToLocal(&result_value));
    int64_t code = 0;
    EXPECT_TRUE(result_value->ToInteger(&code));
    EXPECT_EQ(code, 301);
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
    HandleScope scope(isolate);
    Local<Function> getter =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(getter.IsEmpty());

    Local<Value> bad_argv[1] = {Local<Value>::Cast(Integer::New(isolate, 7))};
    TryCatch try_catch(isolate);
    MaybeLocal<Value> bad_result =
        getter->Call(Context::New(isolate), Local<Value>(), 1, bad_argv);
    EXPECT_TRUE(bad_result.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());

    try_catch.Reset();
    Local<Value> good_argv[1] = {
        Local<Value>::Cast(String::New(isolate, "status"))};
    MaybeLocal<Value> good_result =
        getter->Call(Context::New(isolate), Local<Value>(), 1, good_argv);
    ASSERT_FALSE(good_result.IsEmpty());
    Local<Value> value;
    ASSERT_TRUE(good_result.ToLocal(&value));
    int64_t out = 0;
    EXPECT_TRUE(value->ToInteger(&out));
    EXPECT_EQ(out, 42);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase3Test, Context_Get_Miss_Test) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
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

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase3Test, HostMock_PythonToCppRoundTrip) {
  g_last_status = -1;
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_get_config =
        Function::New(isolate, &HostGetConfig, "Host_GetConfig");
    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_get_config.IsEmpty());
    ASSERT_FALSE(host_set_status.IsEmpty());

    EXPECT_TRUE(context->Set(String::New(isolate, "Host_GetConfig"),
                             Local<Value>::Cast(host_get_config)));
    EXPECT_TRUE(context->Set(String::New(isolate, "Host_SetStatus"),
                             Local<Value>::Cast(host_set_status)));

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
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

    Local<Function> host_get_status =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(host_get_status.IsEmpty());
    EXPECT_TRUE(context->Set(String::New(isolate, "Host_GetStatus"),
                             Local<Value>::Cast(host_get_status)));

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
#endif

}  // namespace saauso
