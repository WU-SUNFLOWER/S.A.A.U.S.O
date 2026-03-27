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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::NewInstance("C"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallInstanceMethodImplicit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C:
  def __init__(self, name):
    self.name = name

  def foo(self):
    print(self.name)

class D:
  pass

o1 = C("baoluo")
o2 = D()
o2.name = "wanxiang"

C.foo(o1)
C.foo(o2)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::NewInstance("baoluo"));
  AppendExpected(expected_printv_result, PyString::NewInstance("wanxiang"));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
