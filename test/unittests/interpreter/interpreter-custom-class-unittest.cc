// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
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

TEST_F(BasicInterpreterTest, DunderAddInvokedByBinaryAdd) {
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

TEST_F(BasicInterpreterTest, DunderLenInvokedByLenBuiltinAndDunderAdd) {
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

v = Vector(3, 4)
print(len(v))
v2 = Vector(1, 2) + Vector(3, 4)
print(len(v2))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(25)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(52)));

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DunderGetSetDelItemInvokedBySubscriptOps) {
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

  auto expected_printv_result = PyList::NewInstance();

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(isolate_->py_none_object()));

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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result,
                 PyString::NewInstance("__getattr__ called"));
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

  auto expected_printv_result = PyList::NewInstance();
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(8)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomClassStrAndReprWorkForPrintAndBuiltinRepr) {
  HandleScope scope;

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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("str(A)"));
  AppendExpected(expected_printv_result, PyString::NewInstance("repr(A)"));
  AppendExpected(expected_printv_result, PyString::NewInstance("repr(B)"));
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_false_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildClassErrors) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
    __build_class__()
except TypeError as e:
    print(str(e))

def body():
    pass

try:
    __build_class__(body, 1)
except TypeError as e:
    print(str(e))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(
      expected_printv_result,
      PyString::NewInstance("__build_class__: not enough arguments"));
  AppendExpected(
      expected_printv_result,
      PyString::NewInstance("__build_class__: name is not a string"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, TypeObjectMroSingleInheritanceIsC3Linearized) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    pass

class B(A):
    pass

class C(B):
    pass

m = C.__mro__
print(len(m))
print(m[0] is C)
print(m[1] is B)
print(m[2] is A)
print(m[3] is object)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, TypeObjectMroDiamondIsC3Linearized) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    pass

class B(A):
    pass

class C(A):
    pass

class D(B, C):
    pass

m = D.__mro__
print(len(m))
print(m[0] is D)
print(m[1] is B)
print(m[2] is C)
print(m[3] is A)
print(m[4] is object)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(5)));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LoadAttrOnTypeObjectFindsPropertyFromBaseMro) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    value = 1

class B(A):
    pass

class C(B):
    value = 3

print(B.value)
print(C.value)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LoadAttrOnTypeObjectMissRaisesAttributeError) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A(object):
    pass

print(A.miss)
)";
  RunScriptExpectExceptionContains(kSource, "AttributeError",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, LoadAttrOnInstanceFallsBackToClassMroProperty) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A(object):
    x = 7

class B(A):
    pass

b = B()
print(b.x)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSubclassInitCanInvokeBaseListInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class L(list):
    def __init__(self, iterable, x):
        list.__init__(self, iterable)
        self.x = x

l = L([1, 2, 3], 100)
print(l[0] + l[1] + l[2] + l.x)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(106)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest,
       ListInitSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = list.__init__
a = list()
f(a, [1, 2, 3])
print(a[0] + a[1] + a[2])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest,
       DictInitSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = dict.__init__
d = dict()
f(d, {"x": 1, "y": 2})
print(d["x"] + d["y"])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinInitBridgeOnInstancePathWorks) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
a = []
a.__init__([4, 5, 6])
print(a[0] + a[1] + a[2])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(15)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinInitBridgeRejectsWrongReceiverType) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
list.__init__(dict(), [1, 2, 3])
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: descriptor",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, ListNewSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = list.__new__
a = f(list)
list.__init__(a, [1, 2, 3])
print(a[0] + a[1] + a[2])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictNewSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = dict.__new__
d = f(dict)
dict.__init__(d, {"x": 1, "y": 2})
print(d["x"] + d["y"])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ObjectNewSlotBridgeSupportsCustomClassReceiver) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    pass

f = object.__new__
a = f(A)
print(isinstance(a, A))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinNewBridgeRejectsWrongReceiverType) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
list.__new__(1)
)";

  RunScriptExpectExceptionContains(
      kSource, "list.__new__() argument 1 must be type, not 'int'",
      kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, CustomClassNewCanParticipateInConstruction) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __new__(cls, x):
        obj = object.__new__(cls)
        obj.value = x + 1
        return obj

    def __init__(self, x):
        self.value = self.value + x

a = A(3)
print(a.value)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BaseNewReceivesDerivedTypeAsCls) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Base:
    def __new__(cls, *args):
        return object.__new__(cls)

class Derived(Base):
    pass

obj = Derived()
print(isinstance(obj, Derived))
print(isinstance(obj, Base))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomNewCanImplementSingletonPattern) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Singleton:
    _instance = None

    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = object.__new__(cls)
        return cls._instance

a = Singleton()
b = Singleton()
print(a is b)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomNewCanReturnOtherTypeInstance) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Dog:
    pass

class Cat:
    pass

class AnimalFactory:
    def __new__(cls, animal_type):
        if animal_type == "dog":
            return Dog()
        elif animal_type == "cat":
            return Cat()
        else:
            return object.__new__(cls)

pet = AnimalFactory("dog")
print(isinstance(pet, Dog))
print(isinstance(pet, AnimalFactory))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_false_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomNewCanCacheInstancesByKey) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Color:
    _cache = {}

    def __new__(cls, name):
        if name not in cls._cache:
            cls._cache[name] = object.__new__(cls)
        return cls._cache[name]

red1 = Color("red")
red2 = Color("red")
blue = Color("blue")
print(red1 is red2)
print(red1 is blue)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_false_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CustomNewDispatchPrefersNearestOverrideInMro) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class Base:
    def __new__(cls, *args):
        obj = object.__new__(cls)
        obj.from_new = "base"
        return obj

class Mid(Base):
    def __new__(cls, *args):
        obj = object.__new__(cls)
        obj.from_new = "mid"
        return obj

class Derived(Mid):
    pass

class DerivedOverride(Mid):
    def __new__(cls, *args):
        obj = object.__new__(cls)
        obj.from_new = "derived"
        return obj

a = Derived()
b = DerivedOverride()
print(a.from_new == "mid")
print(b.from_new == "derived")
print(isinstance(a, Derived))
print(isinstance(b, DerivedOverride))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  AppendExpected(expected_printv_result, handle(isolate_->py_true_object()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, InitMustReturnNone) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self):
        return 1

A()
)";

  RunScriptExpectExceptionContains(kSource,
                                   "__init__() should return None, not 'int'",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest,
       ObjectInitRejectsArgsWhenTypeDefinesNeitherInitNorNew) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class B:
    pass

b = B()
object.__init__(b, 1)
)";

  RunScriptExpectExceptionContains(kSource,
                                   "B.__init__() takes exactly one argument",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, ObjectInitRejectsArgsWhenTypeOverridesInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __init__(self, x):
        object.__init__(self, x)

a = A(1)
)";

  RunScriptExpectExceptionContains(
      kSource, "object.__init__() takes exactly one argument",
      kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, DiamondMroCallDispatchUsesNearestOverride) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
    def __call__(self):
        return "A"

class B(A):
    pass

class C(A):
    def __call__(self):
        return "C"

class D(B, C):
    pass

d = D()
print(d())
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("C"));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
