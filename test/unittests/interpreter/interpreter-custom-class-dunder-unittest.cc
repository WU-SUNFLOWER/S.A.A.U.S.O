// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
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

TEST_F(BasicInterpreterTest, DunderAddInvokedByBinaryAdd) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self, v):
        self.value = v

    def __add__(self, a):
        print("executing operator +")
        return A(self.value + a.value)

a = A(1)
b = A(2)
c = a + b       # executing operator +
print(a.value)   # 1
print(b.value)   # 2
print(c.value)   # 3
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "executing operator +"));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DunderLenInvokedByLenBuiltinAndDunderAdd) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class Vector:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __add__(self, v):
        return Vector(self.x + v.x, self.y + v.y)

    def __len__(self):
        return self.x * self.x + self.y * self.y

v = Vector(3, 4)
print(len(v))
v2 = Vector(1, 2) + Vector(3, 4)
print(len(v2))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(25));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(52));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DunderGetSetDelItemInvokedBySubscriptOps) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self, *args):
        self.attrs = dict()
        n = len(args)
        i = 0
        while (i < n):
            a = args[i]
            i += 1
            b = args[i]
            i += 1
            self.attrs[a] = b
            
    def __getitem__(self, key):
        if key in self.attrs:
            return self.attrs[key]
        else:
            return "Error"

    def __setitem__(self, key, value):
        self.attrs[key] = value

    def __delitem__(self, key):
        del self.attrs[key]

a = A("hello", "hi", "how are you", "fine")
print(a["hello"])           # hi
print(a["how are you"])     # fine
a["one"] = 1
print(a["one"])             # 1
del a["one"]
print(a["one"]) # Error
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result, PyString::New(isolate_, "hi"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "fine"));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "Error"));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomClassStrAndReprWorkForPrintAndBuiltinRepr) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"PY(
class A:
    def __str__(self):
        return "str(A)"

    def __repr__(self):
        return "repr(A)"

class B:
    def __repr__(self):
        return "repr(B)"

a = A()
b = B()
print(str(a))
print(repr(a))
print(str(b))
)PY";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::New(isolate_, "str(A)"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "repr(A)"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "repr(B)"));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
