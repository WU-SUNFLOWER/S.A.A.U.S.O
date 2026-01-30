// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

#include "src/handles/handles.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/native-functions.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"

namespace saauso::internal {

class NativePrintTest : public VmTestBase {};

TEST_F(NativePrintTest, DefaultSepAndEnd) {
  HandleScope scope;

  auto args = PyTuple::NewInstance(2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::NewInstance();

  testing::internal::CaptureStdout();
  Native_Print(Handle<PyObject>::null(), args, kwargs);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a b\n");
}

TEST_F(NativePrintTest, EndParameter) {
  HandleScope scope;

  auto args = PyTuple::NewInstance(2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::NewInstance();
  PyDict::Put(kwargs, ST(end), PyString::NewInstance("!"));

  testing::internal::CaptureStdout();
  Native_Print(Handle<PyObject>::null(), args, kwargs);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a b!");
}

TEST_F(NativePrintTest, EolAliasParameter) {
  HandleScope scope;

  auto args = PyTuple::NewInstance(2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::NewInstance();
  PyDict::Put(kwargs, ST(eol), PyString::NewInstance("??"));

  testing::internal::CaptureStdout();
  Native_Print(Handle<PyObject>::null(), args, kwargs);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a b??");
}

TEST_F(NativePrintTest, SepParameter) {
  HandleScope scope;

  auto args = PyTuple::NewInstance(3);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  args->SetInternal(2, PyString::NewInstance("c"));
  auto kwargs = PyDict::NewInstance();
  PyDict::Put(kwargs, ST(sep), PyString::NewInstance(","));

  testing::internal::CaptureStdout();
  Native_Print(Handle<PyObject>::null(), args, kwargs);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a,b,c\n");
}

TEST_F(NativePrintTest, NoArgsUsesEnd) {
  HandleScope scope;

  auto args = PyTuple::NewInstance(0);
  auto kwargs = PyDict::NewInstance();
  PyDict::Put(kwargs, ST(end), PyString::NewInstance("X"));

  testing::internal::CaptureStdout();
  Native_Print(Handle<PyObject>::null(), args, kwargs);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "X");
}

}  // namespace saauso::internal

