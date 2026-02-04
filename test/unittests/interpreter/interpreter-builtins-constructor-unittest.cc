// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance(""));
  AppendExpected(expected_printv_result, PyString::NewInstance("123"));
  AppendExpected(expected_printv_result, PyString::NewInstance("3.0"));
  AppendExpected(expected_printv_result, PyString::NewInstance("True"));
  AppendExpected(expected_printv_result, PyString::NewInstance("None"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1234)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(255)));
  AppendExpected(expected_printv_result, PyFloat::NewInstance(0.0));
  AppendExpected(expected_printv_result, PyFloat::NewInstance(10.5));
  AppendExpected(expected_printv_result, PyString::NewInstance("nan"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsFloatParseError) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
float("bad")
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "ValueError: could not convert string to float: 'bad'");
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsStrDecodeNotSupported) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
str("abc", "utf-8")
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "TypeError: decoding str is not supported");
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntKeywordNotSupported) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int(x=1)
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "TypeError: int\\(\\) takes no keyword arguments");
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntSmiOverflow) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int("4611686018427387904")
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "OverflowError: int too large to convert to Smi");
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsIntInvalidLiteralWithBase) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
int("0x10", 10)
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "ValueError: invalid literal for int\\(\\) with base 10: '0x10'");
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());

  auto expected_list = PyList::NewInstance();
  PyList::Append(expected_list, handle(PySmi::FromInt(1)));
  PyList::Append(expected_list, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, expected_list);

  auto expected_tuple = PyTuple::NewInstance(2);
  expected_tuple->SetInternal(0, PySmi::FromInt(1));
  expected_tuple->SetInternal(1, PySmi::FromInt(2));
  AppendExpected(expected_printv_result, expected_tuple);

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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuiltinsConstructorsTypeArgCountError) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
type()
)";

  EXPECT_EXIT(
      { RunScript(kSource, kTestFileName); }, ::testing::ExitedWithCode(1),
      "TypeError: type\\(\\) takes 1 or 3 arguments");
}

}  // namespace saauso::internal
