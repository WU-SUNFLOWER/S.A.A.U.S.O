// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/execution/isolate.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyFloatTest : public VmTestBase {};

TEST_F(PyFloatTest, NewInstanceStoresValue) {
  HandleScope scope(isolate_);
  auto f = PyFloat::New(isolate_, 3.14);
  EXPECT_DOUBLE_EQ(f->value(), 3.14);
}

TEST_F(PyFloatTest, IsHeapObjectIsTrueForFloat) {
  HandleScope scope(isolate_);
  Handle<PyObject> f(PyFloat::New(isolate_, 1.0));
  EXPECT_TRUE(IsHeapObject(f));
}

TEST_F(PyFloatTest, ArithmeticBetweenFloats) {
  HandleScope scope(isolate_);

  Handle<PyObject> a(PyFloat::New(isolate_, 10.0));
  Handle<PyObject> b(PyFloat::New(isolate_, 4.0));

  Handle<PyObject> add, sub, mul, div, mod;
  ASSERT_TRUE(PyObject::Add(isolate_, a, b).ToHandle(&add));
  ASSERT_TRUE(PyObject::Sub(isolate_, a, b).ToHandle(&sub));
  ASSERT_TRUE(PyObject::Mul(isolate_, a, b).ToHandle(&mul));
  ASSERT_TRUE(PyObject::Div(isolate_, a, b).ToHandle(&div));
  ASSERT_TRUE(PyObject::Mod(isolate_, a, b).ToHandle(&mod));

  ASSERT_TRUE(IsPyFloat(add));
  ASSERT_TRUE(IsPyFloat(sub));
  ASSERT_TRUE(IsPyFloat(mul));
  ASSERT_TRUE(IsPyFloat(div));
  ASSERT_TRUE(IsPyFloat(mod));

  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(add)->value(), 14.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(sub)->value(), 6.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(mul)->value(), 40.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(div)->value(), 2.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(mod)->value(), 2.0);
}

TEST_F(PyFloatTest, ArithmeticFloatWithSmiBothDirections) {
  HandleScope scope(isolate_);

  Handle<PyObject> f(PyFloat::New(isolate_, 3.5));
  Handle<PyObject> i(PySmi::FromInt(2), isolate_);

  Handle<PyObject> r1, r2, r3, r4, r5, r6;
  ASSERT_TRUE(PyObject::Add(isolate_, f, i).ToHandle(&r1));
  ASSERT_TRUE(PyObject::Sub(isolate_, f, i).ToHandle(&r2));
  ASSERT_TRUE(PyObject::Mul(isolate_, f, i).ToHandle(&r3));
  ASSERT_TRUE(PyObject::Div(isolate_, f, i).ToHandle(&r4));
  ASSERT_TRUE(PyObject::Add(isolate_, i, f).ToHandle(&r5));
  ASSERT_TRUE(PyObject::Mul(isolate_, i, f).ToHandle(&r6));

  ASSERT_TRUE(IsPyFloat(r1));
  ASSERT_TRUE(IsPyFloat(r2));
  ASSERT_TRUE(IsPyFloat(r3));
  ASSERT_TRUE(IsPyFloat(r4));
  ASSERT_TRUE(IsPyFloat(r5));
  ASSERT_TRUE(IsPyFloat(r6));

  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r1)->value(), 5.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r2)->value(), 1.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r3)->value(), 7.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r4)->value(), 1.75);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r5)->value(), 5.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r6)->value(), 7.0);
}

TEST_F(PyFloatTest, ComparisonsFloatWithSmi) {
  HandleScope scope(isolate_);

  Handle<PyObject> f10(PyFloat::New(isolate_, 10.0));
  Handle<PyObject> f11(PyFloat::New(isolate_, 11.0));
  Handle<PyObject> i10(PySmi::FromInt(10), isolate_);

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Equal(isolate_, f10, i10).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::NotEqual(isolate_, f11, i10).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Less(isolate_, f10, f11).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::LessEqual(isolate_, f10, i10).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Greater(isolate_, f11, f10).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::GreaterEqual(isolate_, f10, i10).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

}  // namespace saauso::internal
