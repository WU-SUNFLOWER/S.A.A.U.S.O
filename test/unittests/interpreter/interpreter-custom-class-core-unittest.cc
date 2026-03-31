// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

TEST_F(BasicInterpreterTest, SimpleCustomClass) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    value = 2025

print(A.value)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2025));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildObjectByCustomClass) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s1 = "Hello"
s2 = "World"

class A:
    value = s1 + " " + s2

a = A()

print(A.value)
print(a.value)
print(A.value is a.value)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto s = PyString::New(isolate_, "Hello World");
  AppendExpected(expected_printv_result, s);
  AppendExpected(expected_printv_result, s);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ComplicatedCustomClass) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Vector(object):
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def say(self):
        print(self.x)
        print(self.y)
        print(self.z)

a = Vector(1, 2, 3)
b = Vector(4, 5, 6)

a.say()
b.say()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(4));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(5));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SimpleClassInheritance) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    def say(self):
        print("I am A")

class B(A):
    def say(self):
        print("I am B")

class C(A):
    pass

b = B()
c = C()

b.say()    # "I am B"
c.say()    # "I am A"
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result, PyString::New(isolate_, "I am B"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "I am A"));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, IsInstanceWorksForCustomClassInheritance) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    pass

class B(A):
    pass

b = B()
print(isinstance(b, B))
print(isinstance(b, A))
print(isinstance(b, object))
print(isinstance(b, dict))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
