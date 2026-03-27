// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

// ===== 子类化语义 =====

TEST_F(BasicInterpreterTest, SubclassList) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(list):
    pass

i = 0
last = None
while i < 2000:
    c = C()
    c.append(i)
    c.x = i
    last = c
    i = i + 1

print(len(last))
print(last[0])
print(last.x)
print(1 if last else 0)
last.pop()
print(1 if last else 0)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1999)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1999)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassListCustomInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(list):
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def foo(self):
        return self

c = C(1, 2, 3)
print(c.foo().x)
print(c.foo().y)
print(c.foo().z)
print(len(c.foo()))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
