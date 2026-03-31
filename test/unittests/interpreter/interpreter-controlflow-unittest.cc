// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, IfStatmentTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = 4
y = 2

if x > y:
  print(y - x)
else:
  print(x + y)

if x < y:
  print(1)
elif x == y:
  print(2)
elif x > y:
  print(3)
else:
  print(4)

if None:
  print(True)
else:
  print(False)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, -2));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, PyFalseObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileStatmentTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
a = 1 
b = 0 
i = 0 

print(a)
print(b)
print(i)

while i < 10: 
    print(a)
    t = a 
    a = a + b 
    b = t 
    i = i + 1 
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  int a = 1;
  int b = 0;
  int i = 0;

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(a)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(b)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(i)));

  while (i < 10) {
    AppendExpected(expected_printv_result, handle(PySmi::FromInt(a)));
    int t = a;
    a = a + b;
    b = t;
    i = i + 1;
  }

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileWithBreakTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
while i < 5:
    print(i)
    if i == 3:
        break
    i = i + 1
print(i)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  int i = 0;
  while (i < 5) {
    AppendExpected(expected_printv_result, handle(PySmi::FromInt(i)));
    if (i == 3) {
      break;
    }
    i = i + 1;
  }
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(i)));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileWithContinueTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
while i < 10:
  i = i + 1
  if i < 5:
    continue
  print(i)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  int i = 0;
  while (i < 10) {
    i = i + 1;
    if (i < 5) {
      continue;
    }
    AppendExpected(expected_printv_result, handle(PySmi::FromInt(i)));
  }

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileStatmentTest2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
j = 0
while j < 3:
    i = 0
    while i < 10:
        if i == 3:
            break
        i = i + 1
    j = j + 1
print(i)
print(j)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  int i = 0;
  int j = 0;
  while (j < 3) {
    i = 0;
    while (i < 10) {
      if (i == 3) {
        break;
      }
      i = i + 1;
    }
    j = j + 1;
  }

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(i)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(j)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
