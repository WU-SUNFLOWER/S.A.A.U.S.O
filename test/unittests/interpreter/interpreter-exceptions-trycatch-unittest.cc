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

TEST_F(BasicInterpreterTest, TryExceptCatch) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
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

TEST_F(BasicInterpreterTest, TryExceptHandlerRangeCoversMiddleOfBlock) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  x = 1
  x = x + 1
  raise ValueError
  x = 100
except ValueError:
  print(str(x))
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "2"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptReraise) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  try:
    raise ValueError
  except ValueError:
    print("inner")
    raise
except ValueError:
  print("outer")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "inner"));
  AppendExpected(expected, PyString::New(isolate_, "outer"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except ValueError:
  print("except")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "except"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally2) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise RuntimeError
except ValueError:
  print("value_error")
except RuntimeError:
  print("runtime eroror")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "runtime eroror"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinallyWithExceptionEmitByVm) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  x = len()
except TypeError:
  print("except")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "except"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryFinallyNoException) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  print("try")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "try"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryFinallyReturnRunsFinally) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def f():
  try:
    print("try")
    return "ret"
  finally:
    print("finally")

print(f())
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "try"));
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "ret"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptTupleOfTypes) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise RuntimeError
except (ValueError, RuntimeError):
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptSubclassMatch) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except Exception:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptMismatchPropagatesButFinallyRuns) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  try:
    raise RuntimeError
  except ValueError:
    print("except")
  finally:
    print("finally")
except RuntimeError:
  print("outer")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "finally"));
  AppendExpected(expected, PyString::New(isolate_, "outer"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, CheckExcMatchInvalidTupleElementRaisesTypeError) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  try:
    raise ValueError
  except (ValueError, 1):
    print("unreachable")
except TypeError:
  print("type_error")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "type_error"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, RaiseInstance) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise ValueError()
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

TEST_F(BasicInterpreterTest, RaiseFrom) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
  raise ValueError from RuntimeError
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

}  // namespace saauso::internal
