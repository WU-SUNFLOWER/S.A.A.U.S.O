// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-float.h"
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

TEST_F(BasicInterpreterTest, CompareOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
# group1
print(1 < 3)
print(1 > 3)

# group2
print(1 <= 3)
print(1 >= 3)

# group3
print(3 <= 3)
print(3 >= 3)

# group4
print(6 == 6)
print(6 != 6)
print(1 != 6)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));

  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));

  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FloorDivideOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print(7 // 2)
print((0 - 7) // 2)
print(7 // (0 - 2))
print((0 - 7) // (0 - 2))

print(7.0 // 2)
print(7 // 2.0)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(-4));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(-4));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));

  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 3.0));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 3.0));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, IsOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = "Hello World"
y = x
z = "Hello World"

# group1
# 现代python有优化机制, x、y、z实际上会指向同一个对象
print(x is y)
print(x is z)
print(y is z)

a = "Hello"
b = " "
c = "World"
d = a + b + c
# group2
print(d == x)
print(d is x) # d是拼接得到的，它与x指向的就不是同一个对象了
print(d is not x)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ContainsOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = "Hello SAAUSO World"
y = "USO"
z = "Python"
print(y in x)
print(z in x)
print(x in z)

print(y not in x)
print(z not in x)
print(x not in z)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));

  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));

  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
