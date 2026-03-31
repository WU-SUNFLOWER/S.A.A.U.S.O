// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, SubclassTuple) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class C(tuple):
  pass

c = C()
c.x = 7
print(len(c))
print(c.x)

c2 = C((1, 2, 3))
print(len(c2))
print(c2[1])
print(1 if (2 in c2) else 0)
print(c2.index(2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(0));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(7));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassTupleSurvivesSysgc) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class C(tuple):
  pass

class D:
  pass

def make():
  d1 = D()
  d1.v = 111
  d2 = D()
  d2.v = 222
  c = C((d1,))
  c.x = d2
  return c

c = make()

tmp = []
i = 0
while i < 5000:
  tmp.append([i, i + 1, i + 2])
  i = i + 1

sysgc()
print(c[0].v)
print(c.x.v)

sysgc()
print(c[0].v)
print(c.x.v)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(111));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(222));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(111));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(222));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassTupleCustomInitWithEmptyArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class T(tuple):
  def __init__(self, x, y, z):
    self.x = x
    self.y = y
    self.z = z

t = T()
)";

  RunScriptExpectExceptionContains(
      kSource, "__init__() missing 3 required positional argument",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, SubclassTupleCustomInitWithArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class T(tuple):
  def __init__(self, x, y, z):
    self.x = x
    self.y = y
    self.z = z

t = T(1, 2, 3)
)";

  RunScriptExpectExceptionContains(
      kSource, "tuple expected at most 1 argument, got 3", kTestFileName);
}

TEST_F(BasicInterpreterTest, SubclassTupleCustomInitWithIterableArg) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class T(tuple):
  def __init__(self, x, y, z):
    self.x = x
    self.y = y
    self.z = z

t = T([1, 2, 3])
)";

  RunScriptExpectExceptionContains(
      kSource, "__init__() missing 2 required positional argument",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, SubclassTupleWithoutCustomInitButWithIterableArg) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class T(tuple):
  pass

t = T([1, 2, 3])

for x in t:
  print(x)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
}

TEST_F(BasicInterpreterTest, SubclassStr) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class S(str):
  pass

s = S()
s.x = 7
print(len(s))
print(s.x)

s2 = S("hi")
print(len(s2))
print(s2)
print(s2.upper())
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(0));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(7));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "hi"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "HI"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStrSurvivesSysgc) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class S(str):
  pass

class D:
  pass

def make():
  d = D()
  d.v = 123
  s = S("hello")
  s.x = d
  return s

s = make()

tmp = []
i = 0
while i < 5000:
  tmp.append([i, i + 1, i + 2])
  i = i + 1

sysgc()
print(s.x.v)
print(s)

sysgc()
print(s.x.v)
print(s)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(123));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "hello"));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(123));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "hello"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStrCustomInit) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class S(str):
  def __init__(self, x):
    self.x = x

  def foo(self):
    return self

s = S(7)
print(s.foo().x)
print(len(s.foo()))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(7));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStrCustomInitWithMoreParamsAndArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class S(str):
  def __init__(self, a, b, c, d):
    self.a = a
    self.b = b
    self.c = c
    self.d = d

s1 = S(1,2,3,4)
)";

  RunScriptExpectExceptionContains(
      kSource, "TypeError: str() takes at most 3 arguments (4 given)",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, SubclassStrCustomInitWithMoreParams) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class S(str):
  def __init__(self, a, b, c, d):
    self.a = a
    self.b = b
    self.c = c
    self.d = d

s1 = S("Hello World")
)";

  RunScriptExpectExceptionContains(
      kSource, "__init__() missing 3 required positional argument",
      kTestFileName);
}

}  // namespace saauso::internal
