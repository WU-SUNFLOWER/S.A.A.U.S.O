// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, UnwindAcrossFunctionsCatchInCaller) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def g():
  raise ValueError

def f():
  g()

try:
  f()
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, UnwindFinallyOrderIsInnerToOuter) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def g():
  try:
    raise ValueError
  finally:
    print("g.finally")

def f():
  try:
    g()
  finally:
    print("f.finally")

try:
  f()
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "g.finally"));
  AppendExpected(expected, PyString::New(isolate_, "f.finally"));
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, UnwindMismatchPropagatesAcrossFrames) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def g():
  raise RuntimeError

def f():
  try:
    g()
  except ValueError:
    print("unreachable")
  finally:
    print("f.finally")

try:
  f()
except RuntimeError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "f.finally"));
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ExceptRaisesNewExceptionAndUnwinds) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  try:
    raise ValueError
  except ValueError:
    print("except")
    raise RuntimeError
  finally:
    print("finally")
except RuntimeError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "except"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, FinallyRaisesOverridesOriginalException) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  try:
    raise ValueError
  finally:
    print("finally")
    raise RuntimeError
except RuntimeError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal
