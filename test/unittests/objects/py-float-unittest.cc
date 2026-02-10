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
  HandleScope scope;
  auto f = PyFloat::NewInstance(3.14);
  EXPECT_DOUBLE_EQ(f->value(), 3.14);
}

TEST_F(PyFloatTest, IsGcAbleObjectIsTrueForFloat) {
  HandleScope scope;
  Handle<PyObject> f(PyFloat::NewInstance(1.0));
  EXPECT_TRUE(IsGcAbleObject(f));
}

TEST_F(PyFloatTest, ArithmeticBetweenFloats) {
  HandleScope scope;

  Handle<PyObject> a(PyFloat::NewInstance(10.0));
  Handle<PyObject> b(PyFloat::NewInstance(4.0));

  auto add = PyObject::Add(a, b);
  auto sub = PyObject::Sub(a, b);
  auto mul = PyObject::Mul(a, b);
  auto div = PyObject::Div(a, b);
  auto mod = PyObject::Mod(a, b);

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
  HandleScope scope;

  Handle<PyObject> f(PyFloat::NewInstance(3.5));
  Handle<PyObject> i(PySmi::FromInt(2));

  auto r1 = PyObject::Add(f, i);
  auto r2 = PyObject::Sub(f, i);
  auto r3 = PyObject::Mul(f, i);
  auto r4 = PyObject::Div(f, i);

  auto r5 = PyObject::Add(i, f);
  auto r6 = PyObject::Mul(i, f);

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
  HandleScope scope;

  Handle<PyObject> f10(PyFloat::NewInstance(10.0));
  Handle<PyObject> f11(PyFloat::NewInstance(11.0));
  Handle<PyObject> i10(PySmi::FromInt(10));

  EXPECT_EQ(PyObject::Equal(f10, i10).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::NotEqual(f11, i10).ptr(),
            Isolate::Current()->py_true_object().ptr());

  EXPECT_EQ(PyObject::Less(f10, f11).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::LessEqual(f10, i10).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Greater(f11, f10).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::GreaterEqual(f10, i10).ptr(),
            Isolate::Current()->py_true_object().ptr());
}

}  // namespace saauso::internal
