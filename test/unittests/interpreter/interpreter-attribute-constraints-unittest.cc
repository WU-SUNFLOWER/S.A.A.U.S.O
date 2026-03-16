// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
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

TEST_F(BasicInterpreterTest, BuiltinObjectRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
o = object()
o.x = 233
print(o.x)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'object' object has no attribute 'x'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinStringRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
s = "Hello"
s.x = 123
print(s.x)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'str' object has no attribute 'x'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinTupleRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
t = (1, 2, 3)
t.x = 233
print(t.x)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'tuple' object has no attribute 'x'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinListRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
l = []
l.x = 233
print(l.x)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'list' object has no attribute 'x'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinDictRejectsInstanceSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
d = {}
d.x = 233
print(d.x)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'dict' object has no attribute 'x'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionSupportsDynamicAttributeSetAttr) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
def foo():
    pass

foo.x = 233
print(foo.x)
)";
  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(233)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinSubclassesSupportDynamicAttributes) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class MyList(list):
    pass

class MyDict(dict):
    pass

class MyTuple(tuple):
    pass

class MyStr(str):
    pass

l = MyList()
l.x = 1
print(l.x)

d = MyDict()
d.x = 2
print(d.x)

t = MyTuple()
t.x = 3
print(t.x)

s = MyStr()
s.x = 4
print(s.x)
)";
  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinObjectDictAccessorReadFails) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
o = object()
print(o.__dict__)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'object' object has no attribute '__dict__'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinListDictAccessorReadFails) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
l = []
print(l.__dict__)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'list' object has no attribute '__dict__'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinDictObjectDictAccessorReadFails) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
d = {"x": 1, "y": 2}
print(d.__dict__)
)";
  RunScriptExpectExceptionContains(
      kSource, "AttributeError: 'dict' object has no attribute '__dict__'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest,
       ClassAccessorPriorityOverridesPropertiesShadowing) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

a = A()
a.__dict__["__class__"] = 123
print(a.__dict__["__class__"])
print(a.__class__ is A)
)";
  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(123)));
  AppendExpected(expected_printv_result, PyTrueObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictAccessorPriorityOverridesPropertiesShadowing) {
  HandleScope scope;
  constexpr std::string_view kSource = R"(
class A:
    pass

a = A()
a.__dict__["__dict__"] = 123
print(a.__dict__["__dict__"])
print(isinstance(a.__dict__, dict))
)";
  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(123)));
  AppendExpected(expected_printv_result, PyTrueObject());
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
