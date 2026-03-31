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
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PySmiTest : public VmTestBase {};

TEST_F(PySmiTest, FromIntAndToIntRoundTrip) {
  HandleScope scope(isolate_);

  Handle<PyObject> v0(PySmi::FromInt(0), isolate_);
  Handle<PyObject> v1(PySmi::FromInt(1234), isolate_);
  Handle<PyObject> v2(PySmi::FromInt(-5678), isolate_);

  ASSERT_TRUE(IsPySmi(v0));
  ASSERT_TRUE(IsPySmi(v1));
  ASSERT_TRUE(IsPySmi(v2));

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 0);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 1234);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v2)), -5678);
}

TEST_F(PySmiTest, IsHeapObjectIsFalseForSmi) {
  HandleScope scope(isolate_);
  Handle<PyObject> v(PySmi::FromInt(1), isolate_);
  EXPECT_FALSE(IsHeapObject(v));
}

TEST_F(PySmiTest, FastPathArithmeticBetweenSmis) {
  HandleScope scope(isolate_);

  Handle<PyObject> a(PySmi::FromInt(7), isolate_);
  Handle<PyObject> b(PySmi::FromInt(3), isolate_);

  auto add = PyObject::Add(isolate_, a, b);
  auto sub = PyObject::Sub(isolate_, a, b);
  auto mul = PyObject::Mul(isolate_, a, b);
  auto mod = PyObject::Mod(isolate_, a, b);

  Handle<PyObject> add_obj;
  Handle<PyObject> sub_obj;
  Handle<PyObject> mul_obj;
  Handle<PyObject> mod_obj;
  ASSERT_TRUE(add.ToHandle(&add_obj));
  ASSERT_TRUE(sub.ToHandle(&sub_obj));
  ASSERT_TRUE(mul.ToHandle(&mul_obj));
  ASSERT_TRUE(mod.ToHandle(&mod_obj));

  ASSERT_TRUE(IsPySmi(add_obj));
  ASSERT_TRUE(IsPySmi(sub_obj));
  ASSERT_TRUE(IsPySmi(mul_obj));
  ASSERT_TRUE(IsPySmi(mod_obj));

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(add_obj)), 10);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(sub_obj)), 4);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(mul_obj)), 21);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(mod_obj)), 1);
}

TEST_F(PySmiTest, PythonModMatchesPythonSemanticsForNegative) {
  HandleScope scope(isolate_);

  Handle<PyObject> a(PySmi::FromInt(-7), isolate_);
  Handle<PyObject> b(PySmi::FromInt(3), isolate_);
  auto mod = PyObject::Mod(isolate_, a, b);

  Handle<PyObject> mod_obj;
  ASSERT_TRUE(mod.ToHandle(&mod_obj));
  ASSERT_TRUE(IsPySmi(mod_obj));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(mod_obj)), 2);
}

TEST_F(PySmiTest, FastPathComparisonsBetweenSmis) {
  HandleScope scope(isolate_);

  Handle<PyObject> a(PySmi::FromInt(7), isolate_);
  Handle<PyObject> b(PySmi::FromInt(3), isolate_);
  Handle<PyObject> c(PySmi::FromInt(7), isolate_);

  Handle<PyObject> gt;
  ASSERT_TRUE(PyObject::Greater(isolate_, a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::GreaterEqual(isolate_, a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::Less(isolate_, b, a).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::LessEqual(isolate_, a, c).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, a, c).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::NotEqual(isolate_, a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
}

TEST_F(PySmiTest, MixedArithmeticSmiWithFloat) {
  HandleScope scope(isolate_);

  Handle<PyObject> i(PySmi::FromInt(10), isolate_);
  Handle<PyObject> f(PyFloat::New(isolate_, 0.25));

  Handle<PyObject> r1, r2, r3, r4, r5;
  ASSERT_TRUE(PyObject::Add(isolate_, i, f).ToHandle(&r1));
  ASSERT_TRUE(PyObject::Sub(isolate_, i, f).ToHandle(&r2));
  ASSERT_TRUE(PyObject::Mul(isolate_, i, f).ToHandle(&r3));
  ASSERT_TRUE(PyObject::Div(isolate_, i, f).ToHandle(&r4));
  ASSERT_TRUE(PyObject::Mod(isolate_, i, f).ToHandle(&r5));

  ASSERT_TRUE(IsPyFloat(r1));
  ASSERT_TRUE(IsPyFloat(r2));
  ASSERT_TRUE(IsPyFloat(r3));
  ASSERT_TRUE(IsPyFloat(r4));
  ASSERT_TRUE(IsPyFloat(r5));

  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r1)->value(), 10.25);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r2)->value(), 9.75);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r3)->value(), 2.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r4)->value(), 40.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::cast(r5)->value(), 0.0);
}

TEST_F(PySmiTest, MixedComparisonsSmiWithFloat) {
  HandleScope scope(isolate_);

  Handle<PyObject> i(PySmi::FromInt(10), isolate_);
  Handle<PyObject> f1(PyFloat::New(isolate_, 10.0));
  Handle<PyObject> f2(PyFloat::New(isolate_, 11.0));

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Equal(isolate_, i, f1).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Less(isolate_, i, f2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::LessEqual(isolate_, i, f2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Greater(isolate_, f2, i).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

TEST_F(PySmiTest, MixedEqualitySmiWithBool) {
  HandleScope scope(isolate_);

  Handle<PyObject> one(PySmi::FromInt(1), isolate_);
  Handle<PyObject> zero(PySmi::FromInt(0), isolate_);
  Handle<PyObject> t = PyTrueObject(isolate_);
  Handle<PyObject> f = PyFalseObject(isolate_);

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Equal(isolate_, one, t).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, zero, f).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, one, f).ToHandle(&res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, zero, t).ToHandle(&res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(res)->value());

  ASSERT_TRUE(PyObject::Equal(isolate_, t, one).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, f, zero).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

}  // namespace saauso::internal
