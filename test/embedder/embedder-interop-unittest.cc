// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "saauso-embedder.h"
#include "saauso.h"

namespace saauso {
namespace {

int64_t g_last_status = -1;
std::atomic<int64_t> g_parallel_hits{0};

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

void HostAccumulateParallel(FunctionCallbackInfo& info) {
  int64_t code = 0;
  if (!info.GetIntegerArg(0, &code)) {
    info.ThrowRuntimeError("Host_Acc expects an integer argument");
    return;
  }
  g_parallel_hits.fetch_add(code);
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code)));
}

void HostInnerPlusOne(FunctionCallbackInfo& info) {
  int64_t code = 0;
  if (!info.GetIntegerArg(0, &code)) {
    info.ThrowRuntimeError("Host_InnerPlusOne expects an integer argument");
    return;
  }
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code + 1)));
}

void HostOuterReentrant(FunctionCallbackInfo& info) {
  int64_t code = 0;
  if (!info.GetIntegerArg(0, &code)) {
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
  Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, code))};
  MaybeLocal<Value> result =
      inner->Call(Context::New(isolate), Local<Value>(), 1, argv);
  Local<Value> result_local;
  int64_t inner_value = 0;
  if (result.IsEmpty() || !result.ToLocal(&result_local) ||
      !result_local->ToInteger(&inner_value)) {
    info.ThrowRuntimeError("Host_OuterReentrant failed to call inner callback");
    return;
  }
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(isolate, inner_value + 1)));
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
        HandleScope scope(isolate);
        Local<Function> func =
            Function::New(isolate, &HostAccumulateParallel, "Host_Acc");
        EXPECT_FALSE(func.IsEmpty());
        for (int i = 0; i < kLoops; ++i) {
          Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 1))};
          MaybeLocal<Value> result =
              func->Call(Context::New(isolate), Local<Value>(), 1, argv);
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

TEST(EmbedderPhase4Test, ObjectListTuple_RoundTrip) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Object> obj = Object::New(isolate);
    ASSERT_FALSE(obj.IsEmpty());
    EXPECT_TRUE(obj->Set(String::New(isolate, "name"),
                         Local<Value>::Cast(String::New(isolate, "saauso"))));
    MaybeLocal<Value> name_value = obj->Get(String::New(isolate, "name"));
    ASSERT_FALSE(name_value.IsEmpty());
    Local<Value> name_local;
    ASSERT_TRUE(name_value.ToLocal(&name_local));
    std::string name;
    EXPECT_TRUE(name_local->ToString(&name));
    EXPECT_EQ(name, "saauso");

    Local<List> list = List::New(isolate);
    ASSERT_FALSE(list.IsEmpty());
    EXPECT_TRUE(list->Push(Local<Value>::Cast(Integer::New(isolate, 1))));
    EXPECT_TRUE(list->Push(Local<Value>::Cast(Integer::New(isolate, 2))));
    EXPECT_EQ(list->Length(), 2);
    EXPECT_TRUE(list->Set(1, Local<Value>::Cast(Integer::New(isolate, 99))));
    MaybeLocal<Value> list_item = list->Get(1);
    ASSERT_FALSE(list_item.IsEmpty());
    Local<Value> list_item_local;
    ASSERT_TRUE(list_item.ToLocal(&list_item_local));
    int64_t list_int = 0;
    EXPECT_TRUE(list_item_local->ToInteger(&list_int));
    EXPECT_EQ(list_int, 99);

    Local<Value> tuple_args[2] = {Local<Value>::Cast(String::New(isolate, "x")),
                                  Local<Value>::Cast(Integer::New(isolate, 7))};
    MaybeLocal<Tuple> tuple = Tuple::New(isolate, 2, tuple_args);
    ASSERT_FALSE(tuple.IsEmpty());
    Local<Tuple> tuple_local = tuple.ToLocalChecked();
    EXPECT_EQ(tuple_local->Length(), 2);
    MaybeLocal<Value> tuple_item = tuple_local->Get(0);
    ASSERT_FALSE(tuple_item.IsEmpty());
    Local<Value> tuple_item_local;
    ASSERT_TRUE(tuple_item.ToLocal(&tuple_item_local));
    std::string tuple_s;
    EXPECT_TRUE(tuple_item_local->ToString(&tuple_s));
    EXPECT_EQ(tuple_s, "x");
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, Context_Global_ObjectBridge) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    Local<Object> global = context->Global();
    ASSERT_FALSE(global.IsEmpty());
    EXPECT_TRUE(global->Set(String::New(isolate, "score"),
                            Local<Value>::Cast(Integer::New(isolate, 88))));
    MaybeLocal<Value> score_value = context->Get(String::New(isolate, "score"));
    ASSERT_FALSE(score_value.IsEmpty());
    Local<Value> score_local;
    ASSERT_TRUE(score_value.ToLocal(&score_local));
    int64_t score = 0;
    EXPECT_TRUE(score_local->ToInteger(&score));
    EXPECT_EQ(score, 88);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, MultiContext_VariableIsolation) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context_a = Context::New(isolate);
    Local<Context> context_b = Context::New(isolate);
    ASSERT_FALSE(context_a.IsEmpty());
    ASSERT_FALSE(context_b.IsEmpty());

    EXPECT_TRUE(context_a->Set(String::New(isolate, "score"),
                               Local<Value>::Cast(Integer::New(isolate, 7))));

    MaybeLocal<Value> score_a = context_a->Get(String::New(isolate, "score"));
    ASSERT_FALSE(score_a.IsEmpty());
    Local<Value> score_a_local;
    ASSERT_TRUE(score_a.ToLocal(&score_a_local));
    int64_t score = 0;
    EXPECT_TRUE(score_a_local->ToInteger(&score));
    EXPECT_EQ(score, 7);

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
    HandleScope scope(isolate);
    Local<Context> context_a = Context::New(isolate);
    Local<Context> context_b = Context::New(isolate);
    ASSERT_FALSE(context_a.IsEmpty());
    ASSERT_FALSE(context_b.IsEmpty());

    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_set_status.IsEmpty());
    EXPECT_TRUE(context_a->Set(String::New(isolate, "Host_SetStatus"),
                               Local<Value>::Cast(host_set_status)));

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
    HandleScope scope(isolate);
    Local<Context> context_a = Context::New(isolate);
    Local<Context> context_b = Context::New(isolate);
    ASSERT_FALSE(context_a.IsEmpty());
    ASSERT_FALSE(context_b.IsEmpty());

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
    MaybeLocal<Value> bad_run = maybe_bad_script.ToLocalChecked()->Run(context_a);
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

TEST(EmbedderPhase4Test, Object_CallMethod_Miss_Captured) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Object> obj = Object::New(isolate);
    ASSERT_FALSE(obj.IsEmpty());
    TryCatch try_catch(isolate);
    MaybeLocal<Value> out =
        obj->CallMethod(Context::New(isolate),
                        String::New(isolate, "missing_method"), 0, nullptr);
    EXPECT_TRUE(out.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

#if SAAUSO_ENABLE_CPYTHON_COMPILER
TEST(EmbedderPhase4Test, Object_CallMethod_PythonInstanceMethod) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    Local<Object> obj = Object::New(isolate);
    ASSERT_FALSE(obj.IsEmpty());
    EXPECT_TRUE(obj->Set(String::New(isolate, "k"),
                         Local<Value>::Cast(Integer::New(isolate, 123))));
    TryCatch call_try_catch(isolate);
    Local<Value> get_argv[1] = {Local<Value>::Cast(String::New(isolate, "k"))};
    MaybeLocal<Value> out =
        obj->CallMethod(context, String::New(isolate, "get"), 1, get_argv);
    ASSERT_FALSE(out.IsEmpty());
    EXPECT_FALSE(call_try_catch.HasCaught());
    Local<Value> value;
    ASSERT_TRUE(out.ToLocal(&value));
    int64_t got = 0;
    EXPECT_TRUE(value->ToInteger(&got));
    EXPECT_EQ(got, 123);
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#endif

TEST(EmbedderPhase4Test, FloatBoolean_RoundTrip) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Float> f = Float::New(isolate, 3.5);
    ASSERT_FALSE(f.IsEmpty());
    EXPECT_TRUE(Local<Value>::Cast(f)->IsFloat());
    double fv = 0.0;
    EXPECT_TRUE(Local<Value>::Cast(f)->ToFloat(&fv));
    EXPECT_DOUBLE_EQ(fv, 3.5);

    Local<Boolean> b = Boolean::New(isolate, true);
    ASSERT_FALSE(b.IsEmpty());
    EXPECT_TRUE(Local<Value>::Cast(b)->IsBoolean());
    bool bv = false;
    EXPECT_TRUE(Local<Value>::Cast(b)->ToBoolean(&bv));
    EXPECT_TRUE(bv);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, ListTuple_Get_OutOfRange_SafeReturn) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<List> list = List::New(isolate);
    ASSERT_FALSE(list.IsEmpty());
    EXPECT_TRUE(list->Push(Local<Value>::Cast(Integer::New(isolate, 1))));
    MaybeLocal<Value> list_miss = list->Get(3);
    EXPECT_TRUE(list_miss.IsEmpty());

    Local<Value> tuple_args[1] = {Local<Value>::Cast(Integer::New(isolate, 9))};
    MaybeLocal<Tuple> tuple = Tuple::New(isolate, 1, tuple_args);
    ASSERT_FALSE(tuple.IsEmpty());
    MaybeLocal<Value> tuple_miss = tuple.ToLocalChecked()->Get(2);
    EXPECT_TRUE(tuple_miss.IsEmpty());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, FunctionCall_Reentrant_Safe) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Function> outer =
        Function::New(isolate, &HostOuterReentrant, "Host_OuterReentrant");
    ASSERT_FALSE(outer.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 10))};
    MaybeLocal<Value> result =
        outer->Call(Context::New(isolate), Local<Value>(), 1, argv);
    ASSERT_FALSE(result.IsEmpty());
    Local<Value> value;
    ASSERT_TRUE(result.ToLocal(&value));
    int64_t out = 0;
    EXPECT_TRUE(value->ToInteger(&out));
    EXPECT_EQ(out, 12);
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, Isolate_ThrowException_CapturedByTryCatch) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    TryCatch try_catch(isolate);
    isolate->ThrowException(
        Exception::RuntimeError(String::New(isolate, "boom")));
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());
    std::string message;
    EXPECT_TRUE(try_catch.Exception()->ToString(&message));
    EXPECT_EQ(message, "[RuntimeError] boom");
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

TEST(EmbedderPhase3Test, Callback_EscapeScope_NoGrowth) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    Local<Function> host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(host_set_status.IsEmpty());
    EXPECT_TRUE(context->Set(String::New(isolate, "Host_SetStatus"),
                             Local<Value>::Cast(host_set_status)));
    size_t baseline = isolate->ValueRegistrySizeForTesting();
    Local<String> source = String::New(isolate,
                                       "i = 0\n"
                                       "while i < 5000:\n"
                                       "    Host_SetStatus(i)\n"
                                       "    i = i + 1\n");
    ASSERT_FALSE(source.IsEmpty());
    TryCatch try_catch(isolate);
    MaybeLocal<Script> maybe_script = Script::Compile(isolate, source);
    ASSERT_FALSE(maybe_script.IsEmpty());
    MaybeLocal<Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    ASSERT_FALSE(run_result.IsEmpty());
    EXPECT_FALSE(try_catch.HasCaught());
    size_t after = isolate->ValueRegistrySizeForTesting();
    EXPECT_LE(after, baseline + 8);
  }

  isolate->Dispose();
  Saauso::Dispose();
}
#endif

}  // namespace saauso
