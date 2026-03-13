// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>
#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

std::string TakePendingExceptionMessage(Isolate* isolate) {
  HandleScope scope;
  if (!isolate->HasPendingException()) {
    return std::string();
  }
  Handle<PyString> formatted;
  if (!Runtime_FormatPendingExceptionForStderr().To(&formatted)) {
    isolate->exception_state()->Clear();
    return std::string();
  }
  std::string msg = formatted->ToStdString();
  isolate->exception_state()->Clear();
  return msg;
}

}  // namespace

class AttributeConstraintInterpreterTest : public VmTestBase {};

TEST_F(AttributeConstraintInterpreterTest,
       BuiltinObjectRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
o = object()
o.x = 233
print(o.x)
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  std::string msg = TakePendingExceptionMessage(isolate_);
  EXPECT_NE(msg.find("AttributeError: 'object' object has no attribute 'x'"),
            std::string::npos);
}

TEST_F(AttributeConstraintInterpreterTest,
       BuiltinStringRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
s = "Hello"
s.x = 123
print(s.x)
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  std::string msg = TakePendingExceptionMessage(isolate_);
  EXPECT_NE(msg.find("AttributeError: 'str' object has no attribute 'x'"),
            std::string::npos);
}

TEST_F(AttributeConstraintInterpreterTest, BuiltinTupleRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
t = (1, 2, 3)
t.x = 233
print(t.x)
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  std::string msg = TakePendingExceptionMessage(isolate_);
  EXPECT_NE(msg.find("AttributeError: 'tuple' object has no attribute 'x'"),
            std::string::npos);
}

TEST_F(AttributeConstraintInterpreterTest, BuiltinListRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
l = []
l.x = 233
print(l.x)
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  std::string msg = TakePendingExceptionMessage(isolate_);
  EXPECT_NE(msg.find("AttributeError: 'list' object has no attribute 'x'"),
            std::string::npos);
}

TEST_F(AttributeConstraintInterpreterTest, BuiltinDictRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
d = {}
d.x = 233
print(d.x)
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(run_result.IsNothing());
  std::string msg = TakePendingExceptionMessage(isolate_);
  EXPECT_NE(msg.find("AttributeError: 'dict' object has no attribute 'x'"),
            std::string::npos);
}

TEST_F(AttributeConstraintInterpreterTest,
       FunctionSupportsDynamicAttributeSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
def foo():
    pass

foo.x = 233
if foo.x != 233:
    raise RuntimeError("foo.x mismatch")
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_FALSE(run_result.IsNothing());
  ASSERT_FALSE(isolate_->HasPendingException());
}

TEST_F(AttributeConstraintInterpreterTest,
       BuiltinSubclassesSupportDynamicAttributes) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class MyList(list):
    pass

class MyDict(dict):
    pass

class MyTuple(tuple):
    pass

class MyStr(str):
    pass

l = MyList()
l.x = 1
if l.x != 1:
    raise RuntimeError("list subclass property mismatch")

d = MyDict()
d.x = 2
if d.x != 2:
    raise RuntimeError("dict subclass property mismatch")

t = MyTuple()
t.x = 3
if t.x != 3:
    raise RuntimeError("tuple subclass property mismatch")

s = MyStr()
s.x = 4
if s.x != 4:
    raise RuntimeError("str subclass property mismatch")
)";
  Handle<PyFunction> boilerplate;
  ASSERT_TRUE(Compiler::CompileSource(isolate_, kSource, kTestFileName)
                  .To(&boilerplate));
  auto run_result = isolate_->interpreter()->Run(boilerplate);
  ASSERT_FALSE(run_result.IsNothing());
  ASSERT_FALSE(isolate_->HasPendingException());
}

}  // namespace saauso::internal
