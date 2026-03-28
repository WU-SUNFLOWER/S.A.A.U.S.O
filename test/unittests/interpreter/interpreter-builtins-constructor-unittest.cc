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
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsBasic) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print(str())
print(str(123))
print(str(3.0))
print(str(True))
print(str(None))
print(int())
print(int(" 1_234 "))
print(int("0xff", 0))
print(float())
print(float("1_0.5"))
print(str(float("nan")))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::New(isolate_, ""));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "123"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "3.0"));
  AppendExpected(expected_printv_result, ST(true_symbol));
  AppendExpected(expected_printv_result, ST(none_symbol));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1234)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(255)));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 0.0));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 10.5));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "nan"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsFloatParseError) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
float("bad")
)";
  RunScriptExpectExceptionContains(
      kSource, "ValueError: could not convert string to float: 'bad'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsStrDecodeNotSupported) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
str("abc", "utf-8")
)";
  RunScriptExpectExceptionContains(
      kSource, "TypeError: decoding str is not supported", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntKeywordNotSupported) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int(x=1)
)";
  RunScriptExpectExceptionContains(
      kSource, "TypeError: int() takes no keyword arguments", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntSmiOverflow) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int("4611686018427387904")
)";
  RunScriptExpectExceptionContains(kSource, "int too large to convert to Smi",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntInvalidLiteralWithBase) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int("0x10", 10)
)";
  RunScriptExpectExceptionContains(
      kSource, "ValueError: invalid literal for int()", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsListAndTuple) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
t = (1, 2)
print(tuple(t) is t)

l = [1, 2]
print(list(l) is l)

print(list((1, 2)))
print(tuple([1, 2]))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());

  auto expected_list = PyList::New(isolate_);
  PyList::Append(expected_list, handle(PySmi::FromInt(1)), isolate_);
  PyList::Append(expected_list, handle(PySmi::FromInt(2)), isolate_);
  AppendExpected(expected_printv_result, expected_list);

  auto expected_tuple = PyTuple::New(isolate_, 2);
  expected_tuple->SetInternal(0, PySmi::FromInt(1));
  expected_tuple->SetInternal(1, PySmi::FromInt(2));
  AppendExpected(expected_printv_result, expected_tuple);

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTupleAndTupleLike) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class T(tuple):
  pass

t1 = (1, 2)
t2 = T(t1)
t3 = tuple(t1)

print(t1 == t2 == t3) # True
print(t1 is t2) # False
print(t2 is t3) # False
print(t1 is t3) # True
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  AppendExpected(expected_printv_result, PyTrueObject());

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsType) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print(type(1) is int)
print(type(int) is type)
print(type(type) is type)
C = type("C", (), {})
o = C()
print(type(o) is C)
print(isinstance(o, object))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeCopiesNamespaceDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
ns = {"x": 1}
C = type("C", (), ns)
ns["x"] = 2
print(C.x)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeArgCountError) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type()
)";
  RunScriptExpectExceptionContains(
      kSource, "TypeError: type() takes 1 or 3 arguments", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeArg1MustBeStr) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type(1, (), {})
)";
  RunScriptExpectExceptionContains(kSource, "argument 1 must be str, not 'int'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeArg2MustBeTuple) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type("C", [], {})
)";
  RunScriptExpectExceptionContains(
      kSource, "argument 2 must be tuple, not 'list'", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeArg3MustBeDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type("C", (), [])
)";
  RunScriptExpectExceptionContains(
      kSource, "argument 3 must be dict, not 'list'", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeBasesMustBeTypes) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type("C", (1,), {})
)";
  RunScriptExpectExceptionContains(
      kSource, "TypeError: type() bases must be types", kTestFileName);
}

}  // namespace saauso::internal
