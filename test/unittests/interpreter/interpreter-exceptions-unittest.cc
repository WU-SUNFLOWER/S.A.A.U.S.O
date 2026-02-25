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
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptHandlerRangeCoversMiddleOfBlock) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("2"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptReraise) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("inner"));
  AppendExpected(expected, PyString::NewInstance("outer"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("except"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally2) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("runtime eroror"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryFinallyNoException) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  print("try")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("try"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryFinallyReturnRunsFinally) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("try"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("ret"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptTupleOfTypes) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise RuntimeError
except (ValueError, RuntimeError):
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptSubclassMatch) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except Exception:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptMismatchPropagatesButFinallyRuns) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("outer"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, CheckExcMatchInvalidTupleElementRaisesTypeError) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("type_error"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, RaiseInstance) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError()
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, RaiseFrom) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError from RuntimeError
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, UnwindAcrossFunctionsCatchInCaller) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, UnwindFinallyOrderIsInnerToOuter) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("g.finally"));
  AppendExpected(expected, PyString::NewInstance("f.finally"));
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, UnwindMismatchPropagatesAcrossFrames) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("f.finally"));
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ExceptRaisesNewExceptionAndUnwinds) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("except"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, FinallyRaisesOverridesOriginalException) {
  HandleScope scope;

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

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

// 验证用户自定义迭代器通过 raise StopIteration 终止迭代时，for 循环能正常结束，
// 且不会将 StopIteration 传播到循环外。
TEST_F(BasicInterpreterTest, ForLoopConsumesStopIterationFromCustomIterator) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class MyIter:
  def __init__(self, n):
    self.n = n
    self.i = 0
  def __iter__(self):
    return self
  def __next__(self):
    if self.i >= self.n:
      raise StopIteration
    v = self.i
    self.i += 1
    return v

for x in MyIter(3):
  print(str(x))
print("done")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("0"));
  AppendExpected(expected, PyString::NewInstance("1"));
  AppendExpected(expected, PyString::NewInstance("2"));
  AppendExpected(expected, PyString::NewInstance("done"));
  ExpectPrintResult(expected);
}

// 自定义迭代器无元素时，第一次 __next__ 即 raise StopIteration，for 循环体不执行。
TEST_F(BasicInterpreterTest, ForLoopCustomIteratorEmptyRaisesStopIteration) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class EmptyIter:
  def __iter__(self):
    return self
  def __next__(self):
    raise StopIteration

for x in EmptyIter():
  print("body")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

// 迭代过程中若抛出非 StopIteration 的异常，应正常传播到循环外并被捕获。
TEST_F(BasicInterpreterTest, ForLoopPropagatesNonStopIterationException) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class BadIter:
  def __init__(self):
    self.i = 0
  def __iter__(self):
    return self
  def __next__(self):
    self.i += 1
    if self.i == 2:
      raise ValueError("from iterator")
    if self.i > 3:
      raise StopIteration
    return self.i

try:
  for x in BadIter():
    print(str(x))
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("1"));
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal
