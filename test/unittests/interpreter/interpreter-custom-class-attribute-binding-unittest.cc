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

TEST_F(BasicInterpreterTest, DunderGetAttrSetAttrInterceptsAttributes) {
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
print(b.baz)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, PyNoneObject(isolate_));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomClassAttributeLookupPriorityAndGetAttrHook) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    x = 1
    def __init__(self):
        self.x = 2

    def __getattr__(self, name):
        print("__getattr__ called")
        return 99

a = A()
print(a.x)
print(a.miss)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "__getattr__ called"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(99)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MissingAttributeRaisesAttributeError) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

a = A()
print(a.miss)
)";
  RunScriptExpectExceptionContains(kSource, "AttributeError",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, GetAttrHookMustBeLookedUpFromTypeMro) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

def instance_hook(name):
    return 123

a = A()
a.__getattr__ = instance_hook
print(a.miss)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'A' object has no attribute 'miss'",
      kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, GetAttrExceptionPropagatesToCaller) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    def __getattr__(self, name):
        raise ValueError("boom-from-getattr")

a = A()
print(a.miss)
)";
  RunScriptExpectExceptionContains(kSource, "ValueError: boom-from-getattr",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, InstanceCallableAttributeDoesNotAutoBindReceiver) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

def f(self):
    print(self is a)

a = A()
a.f = f
a.f(a)
)";
  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ClassCallableAttributeAutoBindsReceiver) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

def f(self):
    print(self is a)

a = A()
A.f = f
a.f()
)";
  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ClassFunctionLookupOnTypeReturnsOriginalFunction) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

def f(self):
    pass

A.f = f
print(A.f is f)
)";
  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MethodBindingCallFromInstance) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self, v):
        self.v = v

    def show(self):
        print(self.v)

a = A(7)
f = a.show
f()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MethodCallFromClassWithExplicitSelf) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self, v):
        self.v = v

    def show(self):
        print(self.v)

a = A(8)
g = A.show
g(a)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(8)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ClassFunctionFromTypeRequiresExplicitSelf) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    pass

def f(self):
    pass

A.f = f
ok = False
try:
    A.f()
except TypeError:
    ok = True
print(ok)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
