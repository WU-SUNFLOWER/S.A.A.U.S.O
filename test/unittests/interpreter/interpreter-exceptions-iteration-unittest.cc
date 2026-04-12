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

TEST_F(BasicInterpreterTest, ForLoopConsumesStopIterationFromCustomIterator) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "0"));
  AppendExpected(expected, PyString::New(isolate_, "1"));
  AppendExpected(expected, PyString::New(isolate_, "2"));
  AppendExpected(expected, PyString::New(isolate_, "done"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ForLoopCustomIteratorEmptyRaisesStopIteration) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ForLoopPropagatesNonStopIterationException) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "1"));
  AppendExpected(expected, PyString::New(isolate_, "caught"));
  AppendExpected(expected, PyString::New(isolate_, "after"));
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal
