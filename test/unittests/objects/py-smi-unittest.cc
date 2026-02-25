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

class PySmiTest : public VmTestBase {};

TEST_F(PySmiTest, FromIntAndToIntRoundTrip) {
  HandleScope scope;

  Handle<PyObject> v0(PySmi::FromInt(0));
  Handle<PyObject> v1(PySmi::FromInt(1234));
  Handle<PyObject> v2(PySmi::FromInt(-5678));

  ASSERT_TRUE(IsPySmi(v0));
  ASSERT_TRUE(IsPySmi(v1));
  ASSERT_TRUE(IsPySmi(v2));

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 0);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 1234);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v2)), -5678);
}

TEST_F(PySmiTest, IsGcAbleObjectIsFalseForSmi) {
  HandleScope scope;
  Handle<PyObject> v(PySmi::FromInt(1));
  EXPECT_FALSE(IsGcAbleObject(v));
}

TEST_F(PySmiTest, FastPathArithmeticBetweenSmis) {
  HandleScope scope;

  Handle<PyObject> a(PySmi::FromInt(7));
  Handle<PyObject> b(PySmi::FromInt(3));

  auto add = PyObject::Add(a, b);
  auto sub = PyObject::Sub(a, b);
  auto mul = PyObject::Mul(a, b);
  auto mod = PyObject::Mod(a, b);

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
  HandleScope scope;

  Handle<PyObject> a(PySmi::FromInt(-7));
  Handle<PyObject> b(PySmi::FromInt(3));
  auto mod = PyObject::Mod(a, b);

  Handle<PyObject> mod_obj;
  ASSERT_TRUE(mod.ToHandle(&mod_obj));
  ASSERT_TRUE(IsPySmi(mod_obj));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(mod_obj)), 2);
}

TEST_F(PySmiTest, FastPathComparisonsBetweenSmis) {
  HandleScope scope;

  Handle<PyObject> a(PySmi::FromInt(7));
  Handle<PyObject> b(PySmi::FromInt(3));
  Handle<PyObject> c(PySmi::FromInt(7));

  Handle<PyObject> gt;
  ASSERT_TRUE(PyObject::Greater(a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::GreaterEqual(a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::Less(b, a).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::LessEqual(a, c).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::Equal(a, c).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
  ASSERT_TRUE(PyObject::NotEqual(a, b).ToHandle(&gt));
  EXPECT_TRUE(Handle<PyBoolean>::cast(gt)->value());
}

TEST_F(PySmiTest, MixedArithmeticSmiWithFloat) {
  HandleScope scope;

  Handle<PyObject> i(PySmi::FromInt(10));
  Handle<PyObject> f(PyFloat::NewInstance(0.25));

  Handle<PyObject> r1, r2, r3, r4, r5;
  ASSERT_TRUE(PyObject::Add(i, f).ToHandle(&r1));
  ASSERT_TRUE(PyObject::Sub(i, f).ToHandle(&r2));
  ASSERT_TRUE(PyObject::Mul(i, f).ToHandle(&r3));
  ASSERT_TRUE(PyObject::Div(i, f).ToHandle(&r4));
  ASSERT_TRUE(PyObject::Mod(i, f).ToHandle(&r5));

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
  HandleScope scope;

  Handle<PyObject> i(PySmi::FromInt(10));
  Handle<PyObject> f1(PyFloat::NewInstance(10.0));
  Handle<PyObject> f2(PyFloat::NewInstance(11.0));

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Equal(i, f1).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Less(i, f2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::LessEqual(i, f2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Greater(f2, i).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

TEST_F(PySmiTest, MixedEqualitySmiWithBool) {
  HandleScope scope;

  Handle<PyObject> one(PySmi::FromInt(1));
  Handle<PyObject> zero(PySmi::FromInt(0));
  Handle<PyObject> t = handle(Isolate::Current()->py_true_object());
  Handle<PyObject> f = handle(Isolate::Current()->py_false_object());

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Equal(one, t).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(zero, f).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(one, f).ToHandle(&res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(zero, t).ToHandle(&res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(res)->value());

  ASSERT_TRUE(PyObject::Equal(t, one).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(f, zero).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

}  // namespace saauso::internal
