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

TEST_F(BasicInterpreterTest, TypeObjectMroSingleInheritanceIsC3Linearized) {
  HandleScope scope(isolate_);

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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(4));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, TypeObjectMroDiamondIsC3Linearized) {
  HandleScope scope(isolate_);

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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(5));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LoadAttrOnTypeObjectFindsPropertyFromBaseMro) {
  HandleScope scope(isolate_);

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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LoadAttrOnTypeObjectMissRaisesAttributeError) {
  HandleScope scope(isolate_);
  constexpr std::string_view kSource = R"(
class A(object):
    pass

print(A.miss)
)";
  RunScriptExpectExceptionContains(kSource, "AttributeError",
                                   kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, LoadAttrOnInstanceFallsBackToClassMroProperty) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class A(object):
    x = 7

class B(A):
    pass

b = B()
print(b.x)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(7));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MultipleNativeLayoutBasesRaiseTypeError) {
  HandleScope scope(isolate_);
  constexpr std::string_view kSource = R"(
class C(list, dict):
    pass
)";
  RunScriptExpectExceptionContains(
      kSource, "TypeError: multiple bases have instance lay-out conflict",
      kInterpreterTestFileName);
}

TEST_F(BasicInterpreterTest, ClassAssignmentUpdatesClassAndInstanceLookup) {
  HandleScope scope(isolate_);
  constexpr std::string_view kSource = R"(
class A:
    pass

a = A()
A.x = 42
print(A.x)
print(a.x)
)";
  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(42));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(42));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
