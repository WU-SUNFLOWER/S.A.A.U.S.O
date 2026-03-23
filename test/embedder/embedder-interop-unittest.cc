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
  Maybe<int64_t> code = info.GetIntegerArg(0);
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_SetStatus expects an integer argument");
    return;
  }
  g_last_status = code.ToChecked();
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code.ToChecked())));
}

void HostGetStatusWithValidation(FunctionCallbackInfo& info) {
  Maybe<std::string> key = info.GetStringArg(0);
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
  Maybe<int64_t> code = info.GetIntegerArg(0);
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_Acc expects an integer argument");
    return;
  }
  g_parallel_hits.fetch_add(code.ToChecked());
  info.SetReturnValue(
      Local<Value>::Cast(Integer::New(info.GetIsolate(), code.ToChecked())));
}

void HostInnerPlusOne(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info.GetIntegerArg(0);
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_InnerPlusOne expects an integer argument");
    return;
  }
  info.SetReturnValue(Local<Value>::Cast(
      Integer::New(info.GetIsolate(), code.ToChecked() + 1)));
}

void HostOuterReentrant(FunctionCallbackInfo& info) {
  Maybe<int64_t> code = info.GetIntegerArg(0);
  if (code.IsNothing()) {
    info.ThrowRuntimeError("Host_OuterReentrant expects an integer argument");
    return;
  }
  Isolate* isolate = info.GetIsolate();
  MaybeLocal<Function> maybe_inner =
      Function::New(isolate, &HostInnerPlusOne, "Host_InnerPlusOne");
  if (maybe_inner.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_OuterReentrant failed to create inner callback");
    return;
  }
  Local<Function> inner = maybe_inner.ToLocalChecked();
  MaybeLocal<Context> maybe_context = Context::New(isolate);
  if (maybe_context.IsEmpty()) {
    info.ThrowRuntimeError(
        "Host_OuterReentrant failed to create execution context");
    return;
  }
  Local<Value> argv[1] = {
      Local<Value>::Cast(Integer::New(isolate, code.ToChecked()))};
  MaybeLocal<Value> result =
      inner->Call(maybe_context.ToLocalChecked(), Local<Value>(), 1, argv);
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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 301))};
    MaybeLocal<Value> result =
        func->Call(maybe_context.ToLocalChecked(), Local<Value>(), 1, argv);
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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());

    Local<Value> bad_argv[1] = {Local<Value>::Cast(Integer::New(isolate, 7))};
    TryCatch try_catch(isolate);
    MaybeLocal<Value> bad_result = getter->Call(maybe_context.ToLocalChecked(),
                                                Local<Value>(), 1, bad_argv);
    EXPECT_TRUE(bad_result.IsEmpty());
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());

    try_catch.Reset();
    Local<Value> good_argv[1] = {
        Local<Value>::Cast(String::New(isolate, "status"))};
    MaybeLocal<Value> good_result = getter->Call(maybe_context.ToLocalChecked(),
                                                 Local<Value>(), 1, good_argv);
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

TEST(EmbedderPhase3Test, Context_Get_Miss_Test) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);

    HandleScope scope(isolate);
    
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
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
        Isolate::Scope isolate_scope(isolate);
        HandleScope scope(isolate);
        MaybeLocal<Function> maybe_func =
            Function::New(isolate, &HostAccumulateParallel, "Host_Acc");
        EXPECT_FALSE(maybe_func.IsEmpty());
        Local<Function> func = maybe_func.ToLocalChecked();
        MaybeLocal<Context> maybe_context = Context::New(isolate);
        EXPECT_FALSE(maybe_context.IsEmpty());
        for (int i = 0; i < kLoops; ++i) {
          Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 1))};
          MaybeLocal<Value> result = func->Call(maybe_context.ToLocalChecked(),
                                                Local<Value>(), 1, argv);
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
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Object> maybe_obj = Object::New(isolate);
    ASSERT_FALSE(maybe_obj.IsEmpty());
    Local<Object> obj = maybe_obj.ToLocalChecked();
    EXPECT_TRUE(obj->Set(String::New(isolate, "name"),
                         Local<Value>::Cast(String::New(isolate, "saauso")))
                    .IsJust());
    MaybeLocal<Value> name_value = obj->Get(String::New(isolate, "name"));
    ASSERT_FALSE(name_value.IsEmpty());
    Local<Value> name_local;
    ASSERT_TRUE(name_value.ToLocal(&name_local));
    Maybe<std::string> name = name_local->ToString();
    ASSERT_FALSE(name.IsNothing());
    EXPECT_EQ(name.ToChecked(), "saauso");

    MaybeLocal<List> maybe_list = List::New(isolate);
    ASSERT_FALSE(maybe_list.IsEmpty());
    Local<List> list = maybe_list.ToLocalChecked();
    EXPECT_TRUE(
        list->Push(Local<Value>::Cast(Integer::New(isolate, 1))).IsJust());
    EXPECT_TRUE(
        list->Push(Local<Value>::Cast(Integer::New(isolate, 2))).IsJust());
    EXPECT_EQ(list->Length(), 2);
    EXPECT_TRUE(
        list->Set(1, Local<Value>::Cast(Integer::New(isolate, 99))).IsJust());
    MaybeLocal<Value> list_item = list->Get(1);
    ASSERT_FALSE(list_item.IsEmpty());
    Local<Value> list_item_local;
    ASSERT_TRUE(list_item.ToLocal(&list_item_local));
    Maybe<int64_t> list_int = list_item_local->ToInteger();
    ASSERT_FALSE(list_int.IsNothing());
    EXPECT_EQ(list_int.ToChecked(), 99);

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
    Maybe<std::string> tuple_s = tuple_item_local->ToString();
    ASSERT_FALSE(tuple_s.IsNothing());
    EXPECT_EQ(tuple_s.ToChecked(), "x");
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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
    MaybeLocal<Object> maybe_global = context->Global();
    ASSERT_FALSE(maybe_global.IsEmpty());
    Local<Object> global = maybe_global.ToLocalChecked();
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

    MaybeLocal<Function> maybe_host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(maybe_host_set_status.IsEmpty());
    Local<Function> host_set_status = maybe_host_set_status.ToLocalChecked();
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

TEST(EmbedderPhase4Test, Object_CallMethod_Miss_Captured) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Object> maybe_obj = Object::New(isolate);
    ASSERT_FALSE(maybe_obj.IsEmpty());
    Local<Object> obj = maybe_obj.ToLocalChecked();
    TryCatch try_catch(isolate);
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    MaybeLocal<Value> out =
        obj->CallMethod(maybe_context.ToLocalChecked(),
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
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
    MaybeLocal<Object> maybe_obj = Object::New(isolate);
    ASSERT_FALSE(maybe_obj.IsEmpty());
    Local<Object> obj = maybe_obj.ToLocalChecked();
    EXPECT_TRUE(obj->Set(String::New(isolate, "k"),
                         Local<Value>::Cast(Integer::New(isolate, 123)))
                    .IsJust());
    TryCatch call_try_catch(isolate);
    Local<Value> get_argv[1] = {Local<Value>::Cast(String::New(isolate, "k"))};
    MaybeLocal<Value> out =
        obj->CallMethod(context, String::New(isolate, "get"), 1, get_argv);
    ASSERT_FALSE(out.IsEmpty());
    EXPECT_FALSE(call_try_catch.HasCaught());
    Local<Value> value;
    ASSERT_TRUE(out.ToLocal(&value));
    Maybe<int64_t> got = value->ToInteger();
    ASSERT_FALSE(got.IsNothing());
    EXPECT_EQ(got.ToChecked(), 123);
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
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Float> f = Float::New(isolate, 3.5);
    ASSERT_FALSE(f.IsEmpty());
    EXPECT_TRUE(Local<Value>::Cast(f)->IsFloat());
    Maybe<double> fv = Local<Value>::Cast(f)->ToFloat();
    ASSERT_FALSE(fv.IsNothing());
    EXPECT_DOUBLE_EQ(fv.ToChecked(), 3.5);

    Local<Boolean> b = Boolean::New(isolate, true);
    ASSERT_FALSE(b.IsEmpty());
    EXPECT_TRUE(Local<Value>::Cast(b)->IsBoolean());
    Maybe<bool> bv = Local<Value>::Cast(b)->ToBoolean();
    ASSERT_FALSE(bv.IsNothing());
    EXPECT_TRUE(bv.ToChecked());
  }

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(EmbedderPhase4Test, ListTuple_Get_OutOfRange_SafeReturn) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<List> maybe_list = List::New(isolate);
    ASSERT_FALSE(maybe_list.IsEmpty());
    Local<List> list = maybe_list.ToLocalChecked();
    EXPECT_TRUE(
        list->Push(Local<Value>::Cast(Integer::New(isolate, 1))).IsJust());
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
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Function> maybe_outer =
        Function::New(isolate, &HostOuterReentrant, "Host_OuterReentrant");
    ASSERT_FALSE(maybe_outer.IsEmpty());
    Local<Function> outer = maybe_outer.ToLocalChecked();
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Value> argv[1] = {Local<Value>::Cast(Integer::New(isolate, 10))};
    MaybeLocal<Value> result =
        outer->Call(maybe_context.ToLocalChecked(), Local<Value>(), 1, argv);
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

TEST(EmbedderPhase4Test, Isolate_ThrowException_CapturedByTryCatch) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    TryCatch try_catch(isolate);
    MaybeLocal<Value> error =
        Exception::RuntimeError(String::New(isolate, "boom"));
    ASSERT_FALSE(error.IsEmpty());
    isolate->ThrowException(error.ToLocalChecked());
    EXPECT_TRUE(try_catch.HasCaught());
    EXPECT_FALSE(try_catch.Exception().IsEmpty());
    Local<Value> exception;
    ASSERT_TRUE(try_catch.Exception().ToLocal(&exception));
    Maybe<std::string> message = exception->ToString();
    ASSERT_FALSE(message.IsNothing());
    EXPECT_EQ(message.ToChecked(), "[RuntimeError] boom");
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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();

    MaybeLocal<Function> maybe_host_get_config =
        Function::New(isolate, &HostGetConfig, "Host_GetConfig");
    MaybeLocal<Function> maybe_host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(maybe_host_get_config.IsEmpty());
    ASSERT_FALSE(maybe_host_set_status.IsEmpty());
    Local<Function> host_get_config = maybe_host_get_config.ToLocalChecked();
    Local<Function> host_set_status = maybe_host_set_status.ToLocalChecked();

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
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();

    MaybeLocal<Function> maybe_host_get_status =
        Function::New(isolate, &HostGetStatusWithValidation, "Host_GetStatus");
    ASSERT_FALSE(maybe_host_get_status.IsEmpty());
    Local<Function> host_get_status = maybe_host_get_status.ToLocalChecked();
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

TEST(EmbedderPhase3Test, Callback_EscapeScope_NoGrowth) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    MaybeLocal<Context> maybe_context = Context::New(isolate);
    ASSERT_FALSE(maybe_context.IsEmpty());
    Local<Context> context = maybe_context.ToLocalChecked();
    MaybeLocal<Function> maybe_host_set_status =
        Function::New(isolate, &HostSetStatus, "Host_SetStatus");
    ASSERT_FALSE(maybe_host_set_status.IsEmpty());
    Local<Function> host_set_status = maybe_host_set_status.ToLocalChecked();
    EXPECT_TRUE(context
                    ->Set(String::New(isolate, "Host_SetStatus"),
                          Local<Value>::Cast(host_set_status))
                    .IsJust());
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

// TODO: Isolate模型完善后开放
// TEST(EmbedderContractDeathTest, WrongThreadInvocationShouldDie) {
//   ASSERT_DEATH_IF_SUPPORTED(
//       {
//         Saauso::Initialize();
//         Isolate* isolate = Isolate::New();
//         {
//           HandleScope scope(isolate);
//           std::thread worker([&]() {
//             Local<String> value = String::New(isolate, "cross-thread");
//             (void)value;
//           });
//           worker.join();
//         }
//         isolate->Dispose();
//         Saauso::Dispose();
//       },
//       "");
// }
#endif

}  // namespace saauso
