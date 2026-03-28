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

TEST_F(BasicInterpreterTest, IndexMethod) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s = "xxyyzz"
print(s.index("yy"))
print(s.index("yy", 1))
print(s.index("", 3))
print(s.index("yy", 0, 4))

l = [1, 2, 1, 3]
print(l.index(1))
print(l.index(1, 1))
print(l.index(1, 1, 3))

t = (1, 2, 1, 3)
print(t.index(1))
print(t.index(1, 1))
print(t.index(1, 1, 3))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FindAndRfindMethods) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s = "xxyyzz"
print(s.find("yy"))
print(s.find("aa"))
print(s.find("yy", 1))
print(s.find("", 3))
print(s.find("yy", 0, 4))

print(s.rfind("yy"))
print(s.rfind("y"))
print(s.rfind("aa"))
print(s.rfind("", 0, 4))
print(s.rfind("yy", 0, 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(-1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));

  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(-1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(-1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SplitMethod) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print("a b  c".split())
print(" a  b ".split(maxsplit=1))
print("a,,b,".split(","))
print("".split(","))
print("a,b".split(sep=","))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto v0 = PyList::New(isolate_);
  PyList::Append(v0, PyString::New(isolate_, "a"), isolate_);
  PyList::Append(v0, PyString::New(isolate_, "b"), isolate_);
  PyList::Append(v0, PyString::New(isolate_, "c"), isolate_);
  AppendExpected(expected_printv_result, v0);

  auto v1 = PyList::New(isolate_);
  PyList::Append(v1, PyString::New(isolate_, "a"), isolate_);
  PyList::Append(v1, PyString::New(isolate_, "b"), isolate_);
  AppendExpected(expected_printv_result, v1);

  auto v2 = PyList::New(isolate_);
  PyList::Append(v2, PyString::New(isolate_, "a"), isolate_);
  PyList::Append(v2, PyString::New(isolate_, ""), isolate_);
  PyList::Append(v2, PyString::New(isolate_, "b"), isolate_);
  PyList::Append(v2, PyString::New(isolate_, ""), isolate_);
  AppendExpected(expected_printv_result, v2);

  auto v3 = PyList::New(isolate_);
  PyList::Append(v3, PyString::New(isolate_, ""), isolate_);
  AppendExpected(expected_printv_result, v3);

  auto v4 = PyList::New(isolate_);
  PyList::Append(v4, PyString::New(isolate_, "a"), isolate_);
  PyList::Append(v4, PyString::New(isolate_, "b"), isolate_);
  AppendExpected(expected_printv_result, v4);

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SplitMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kEmptySep = R"(
"abc".split("")
)";
  RunScriptExpectExceptionContains(kEmptySep, "ValueError: empty separator",
                                   kTestFileName);

  constexpr std::string_view kUnexpectedKeyword = R"(
"a".split(foo=1)
)";
  RunScriptExpectExceptionContains(kUnexpectedKeyword,
                                   "str.split() got an unexpected keyword "
                                   "argument",
                                   kTestFileName);

  constexpr std::string_view kMultipleValues = R"(
"a".split(",", sep=",")
)";
  RunScriptExpectExceptionContains(
      kMultipleValues, "str.split() got multiple values for argument 'sep'",
      kTestFileName);
}

TEST_F(BasicInterpreterTest, JoinMethod) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print(",".join(["a", "b"]))
print("".join(("a", "b", "c")))
print("x".join([]))
print("x".join(["a"]))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::New(isolate_, "a,b"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "abc"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, ""));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "a"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, JoinMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kElementNotStr = R"(
print(",".join(["a", 1]))
)";
  RunScriptExpectExceptionContains(
      kElementNotStr, "TypeError: sequence item 1: expected str instance",
      kTestFileName);

  constexpr std::string_view kKeyword = R"(
"-".join(iterable=["a", "b"])
)";
  RunScriptExpectExceptionContains(
      kKeyword, "str.join() takes no keyword arguments", kTestFileName);
}

TEST_F(BasicInterpreterTest, IndexMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kStrNotFound = R"(
"abc".index("d")
)";
  RunScriptExpectExceptionContains(kStrNotFound, "substring not found",
                                   kTestFileName);

  constexpr std::string_view kListNotFound = R"(
[1].index(2)
)";
  RunScriptExpectExceptionContains(kListNotFound, "ValueError", kTestFileName);

  constexpr std::string_view kTupleNotFound = R"(
(1,).index(2)
)";
  RunScriptExpectExceptionContains(
      kTupleNotFound, "tuple.index(x): x not in tuple", kTestFileName);

  constexpr std::string_view kStrKeyword = R"(
"abc".index(sub="a")
)";
  RunScriptExpectExceptionContains(
      kStrKeyword, "str.index() takes no keyword arguments", kTestFileName);
}

TEST_F(BasicInterpreterTest, FindAndRfindKeywordErrors) {
  HandleScope scope;

  constexpr std::string_view kFindKeyword = R"(
"abc".find(sub="a")
)";
  RunScriptExpectExceptionContains(
      kFindKeyword, "str.find() takes no keyword arguments", kTestFileName);

  constexpr std::string_view kRfindKeyword = R"(
"abc".rfind(sub="a")
)";
  RunScriptExpectExceptionContains(
      kRfindKeyword, "str.rfind() takes no keyword arguments", kTestFileName);
}

}  // namespace saauso::internal
