// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/objects/py-float.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/runtime/universe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

// Smi（小整数）是一种 Tagged 编码，并不在堆上分配对象实体。
// 这里重点验证：
// - 编码/解码正确
// - PyObject 快速路径（Smi-Smi）运算正确
// - 与 float 的混合运算分发正确
class PySmiTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { Universe::Genesis(); }
  static void TearDownTestSuite() { Universe::Destroy(); }
};

TEST_F(PySmiTest, FromIntAndToIntRoundTrip) {
  HandleScope scope;

  Handle<PyObject> v0(PySmi::FromInt(0));
  Handle<PyObject> v1(PySmi::FromInt(1234));
  Handle<PyObject> v2(PySmi::FromInt(-5678));

  ASSERT_TRUE(IsPySmi(v0));
  ASSERT_TRUE(IsPySmi(v1));
  ASSERT_TRUE(IsPySmi(v2));

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(v0)), 0);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(v1)), 1234);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(v2)), -5678);
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

  ASSERT_TRUE(IsPySmi(add));
  ASSERT_TRUE(IsPySmi(sub));
  ASSERT_TRUE(IsPySmi(mul));
  ASSERT_TRUE(IsPySmi(mod));

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(add)), 10);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(sub)), 4);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(mul)), 21);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(mod)), 1);
}

TEST_F(PySmiTest, PythonModMatchesPythonSemanticsForNegative) {
  HandleScope scope;

  Handle<PyObject> a(PySmi::FromInt(-7));
  Handle<PyObject> b(PySmi::FromInt(3));
  auto mod = PyObject::Mod(a, b);

  ASSERT_TRUE(IsPySmi(mod));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::Cast(mod)), 2);
}

TEST_F(PySmiTest, FastPathComparisonsBetweenSmis) {
  HandleScope scope;

  Handle<PyObject> a(PySmi::FromInt(7));
  Handle<PyObject> b(PySmi::FromInt(3));
  Handle<PyObject> c(PySmi::FromInt(7));

  EXPECT_EQ(PyObject::Greater(a, b).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::GreaterEqual(a, b).ptr(),
            Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::Less(b, a).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::LessEqual(a, c).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::Equal(a, c).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::NotEqual(a, b).ptr(), Universe::py_true_object_.ptr());
}

TEST_F(PySmiTest, MixedArithmeticSmiWithFloat) {
  HandleScope scope;

  Handle<PyObject> i(PySmi::FromInt(10));
  Handle<PyObject> f(PyFloat::NewInstance(0.25));

  auto r1 = PyObject::Add(i, f);
  auto r2 = PyObject::Sub(i, f);
  auto r3 = PyObject::Mul(i, f);
  auto r4 = PyObject::Div(i, f);
  auto r5 = PyObject::Mod(i, f);

  ASSERT_TRUE(IsPyFloat(r1));
  ASSERT_TRUE(IsPyFloat(r2));
  ASSERT_TRUE(IsPyFloat(r3));
  ASSERT_TRUE(IsPyFloat(r4));
  ASSERT_TRUE(IsPyFloat(r5));

  EXPECT_DOUBLE_EQ(Handle<PyFloat>::Cast(r1)->value(), 10.25);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::Cast(r2)->value(), 9.75);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::Cast(r3)->value(), 2.5);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::Cast(r4)->value(), 40.0);
  EXPECT_DOUBLE_EQ(Handle<PyFloat>::Cast(r5)->value(), 0.0);
}

TEST_F(PySmiTest, MixedComparisonsSmiWithFloat) {
  HandleScope scope;

  Handle<PyObject> i(PySmi::FromInt(10));
  Handle<PyObject> f1(PyFloat::NewInstance(10.0));
  Handle<PyObject> f2(PyFloat::NewInstance(11.0));

  EXPECT_EQ(PyObject::Equal(i, f1).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::Less(i, f2).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::LessEqual(i, f2).ptr(), Universe::py_true_object_.ptr());
  EXPECT_EQ(PyObject::Greater(f2, i).ptr(), Universe::py_true_object_.ptr());
}

}  // namespace saauso::internal
