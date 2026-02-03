// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2025)));
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

  auto expected_printv_result = PyList::NewInstance();

  auto s = PyString::NewInstance("Hello World");
  AppendExpected(expected_printv_result, s);
  AppendExpected(expected_printv_result, s);
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));

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

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(5)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, OperatorOverloading1) {
  HandleScope scope;

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

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result,
                 PyString::NewInstance("executing operator +"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, OperatorOverloading2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Vector:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __add__(self, v):
        return Vector(self.x + v.x, self.y + v.y)

    def __len__(self):
        return self.x * self.x + self.y * self.y

print(len(Vector(3, 4)))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(25)));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, OperatorOverloading3) {
  HandleScope scope;

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

    def __str__(self):
        return "object of A"

a = A("hello", "hi", "how are you", "fine")
print(a["hello"])           # hi
print(a["how are you"])     # fine
a["one"] = 1
print(a["one"])             # 1
del a["one"]
print(a["one"]) # Error
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, PyString::NewInstance("hi"));
  AppendExpected(expected_printv_result, PyString::NewInstance("fine"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, PyString::NewInstance("Error"));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, OperatorOverloading4) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
keys = []
values = []

class B(object):
    def __setattr__(self, k, v):
        if k in keys:
            index = keys.index(k)
            values[index] = v
        else:
            keys.append(k)
            values.append(v)

    def __getattr__(self, k):
        if k in keys:
            index = keys.index(k)
            return values[index]
        else:
            return None

b = B()
b.foo = 1
b.bar = 2
print(b.foo)
print(b.bar)
b.foo = 3
print(b.foo)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));

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

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, PyString::NewInstance("I am B"));
  AppendExpected(expected_printv_result, PyString::NewInstance("I am A"));

  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
