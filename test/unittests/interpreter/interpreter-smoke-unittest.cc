// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, HelloWorld) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print("Hello World")
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("Hello World"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MakeAndCallFunction) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def foo():
  print("foo")
print("before")
foo()
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("before"));
  AppendExpected(expected_printv_result, PyString::NewInstance("foo"));
  AppendExpected(expected_printv_result, PyString::NewInstance("after"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, RunReturnsJustOnSuccess) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print("ok")
)";

  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate(), kSource, kTestFileName)
                  .To(&boilerplate));

  auto run_result = isolate()->interpreter()->Run(boilerplate);
  ASSERT_FALSE(run_result.IsNothing());
  ASSERT_FALSE(isolate()->HasPendingException());
}

TEST_F(BasicInterpreterTest, RunReturnsNothingOnUnhandledException) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
raise RuntimeError("boom")
)";

  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate(), kSource, kTestFileName)
                  .To(&boilerplate));

  auto run_result = isolate()->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  ASSERT_TRUE(isolate()->HasPendingException());

  std::string msg = ExpectedAndTakePendingExceptionMessage();
  ASSERT_FALSE(msg.empty());
  EXPECT_NE(msg.find("RuntimeError"), std::string::npos);
}

}  // namespace saauso::internal
