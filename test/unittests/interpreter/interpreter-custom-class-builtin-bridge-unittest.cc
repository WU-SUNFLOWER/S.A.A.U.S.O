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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(106)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, InitSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = list.__init__
a = list()
f(a, [1, 2, 3])
print(a[0] + a[1] + a[2])

g = dict.__init__
d = dict()
g(d, {"x": 1, "y": 2})
print(d["x"] + d["y"])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
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

  auto expected_printv_result = PyList::New(isolate_);
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

TEST_F(BasicInterpreterTest, NewSlotBridgeStaysCallableAfterAttributeLoad) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = list.__new__
a = f(list)
list.__init__(a, [1, 2, 3])
print(a[0] + a[1] + a[2])

g = dict.__new__
d = g(dict)
dict.__init__(d, {"x": 1, "y": 2})
print(d["x"] + d["y"])
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
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

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinNewSlotSupportsInstanceStyleAccess) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class D(dict):
  pass

d = {"x": 1, "y": 2}
a = d.__new__(D)
print(isinstance(a, D))
print(isinstance(a, dict))

class L(list):
  pass

l = [1, 2, 3]
b = l.__new__(L)
print(isinstance(b, L))
print(isinstance(b, list))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
  AppendExpected(expected_printv_result, PyTrueObject(isolate_));
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

}  // namespace saauso::internal
