// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"
#include "gtest/gtest.h"

namespace saauso {
namespace {

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
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());
    MaybeLocal<Value> out = obj->CallMethod(
        context, String::New(isolate, "missing_method"), 0, nullptr);
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
    Local<Context> context = Context::New(isolate);
    ASSERT_FALSE(context.IsEmpty());

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

}  // namespace
}  // namespace saauso
