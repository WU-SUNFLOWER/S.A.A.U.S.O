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
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, CallMethodOfBuiltinType) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s = "hello"
t = s.upper()
print(s)
print(t)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("hello"));
  AppendExpected(expected_printv_result, PyString::NewInstance("HELLO"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildList) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
lst = [1, "hello"]
print(lst)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance(2);
  PyList::Append(list, handle(PySmi::FromInt(1)));
  PyList::Append(list, PyString::NewInstance("hello"));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

// 构建长的list字面量会用到Python3的新字节码LIST_EXTEND，
// 因此这里设置一个独立的单测！
TEST_F(BasicInterpreterTest, BuildLongList) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
lst = [1, 2, 3, 4, 5, 6, "hello"]
print(lst)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance(2);
  PyList::Append(list, handle(PySmi::FromInt(1)));
  PyList::Append(list, handle(PySmi::FromInt(2)));
  PyList::Append(list, handle(PySmi::FromInt(3)));
  PyList::Append(list, handle(PySmi::FromInt(4)));
  PyList::Append(list, handle(PySmi::FromInt(5)));
  PyList::Append(list, handle(PySmi::FromInt(6)));
  PyList::Append(list, PyString::NewInstance("hello"));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSubscrAccess) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
lst = [233, "world"]
print(lst[0])
print(lst[1])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(233)));
  AppendExpected(expected_printv_result, PyString::NewInstance("world"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListContainOp) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = ["hello", "world"]
print("hello" in l)
print("world" not in l)
print("python" in l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListAppend) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = []
l.append(123)
l.append(3.14)
l.append("python")
print(len(l))
print(l[0], l[1], l[2])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(123)));
  AppendExpected(expected_printv_result, PyFloat::NewInstance(3.14));
  AppendExpected(expected_printv_result, PyString::NewInstance("python"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListInsert) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [1, 2]
l.insert(0, 3)
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance();
  PyList::Append(list, handle(PySmi::FromInt(3)));
  PyList::Append(list, handle(PySmi::FromInt(1)));
  PyList::Append(list, handle(PySmi::FromInt(2)));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

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

  auto expected_printv_result = PyList::NewInstance();
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

  auto expected_printv_result = PyList::NewInstance();
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

  auto expected_printv_result = PyList::NewInstance();

  auto v0 = PyList::NewInstance();
  PyList::Append(v0, PyString::NewInstance("a"));
  PyList::Append(v0, PyString::NewInstance("b"));
  PyList::Append(v0, PyString::NewInstance("c"));
  AppendExpected(expected_printv_result, v0);

  auto v1 = PyList::NewInstance();
  PyList::Append(v1, PyString::NewInstance("a"));
  PyList::Append(v1, PyString::NewInstance("b"));
  AppendExpected(expected_printv_result, v1);

  auto v2 = PyList::NewInstance();
  PyList::Append(v2, PyString::NewInstance("a"));
  PyList::Append(v2, PyString::NewInstance(""));
  PyList::Append(v2, PyString::NewInstance("b"));
  PyList::Append(v2, PyString::NewInstance(""));
  AppendExpected(expected_printv_result, v2);

  auto v3 = PyList::NewInstance();
  PyList::Append(v3, PyString::NewInstance(""));
  AppendExpected(expected_printv_result, v3);

  auto v4 = PyList::NewInstance();
  PyList::Append(v4, PyString::NewInstance("a"));
  PyList::Append(v4, PyString::NewInstance("b"));
  AppendExpected(expected_printv_result, v4);

  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SplitMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kEmptySep = R"(
"abc".split("")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kEmptySep, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  auto formatted = Runtime_FormatPendingExceptionForStderr();
  std::string message(formatted->buffer(),
                      static_cast<size_t>(formatted->length()));
  EXPECT_NE(message.find("ValueError: empty separator"), std::string::npos);
  isolate()->exception_state()->Clear();

  constexpr std::string_view kUnexpectedKeyword = R"(
"a".split(foo=1)
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kUnexpectedKeyword, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.split() got an unexpected keyword argument"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();

  constexpr std::string_view kMultipleValues = R"(
"a".split(",", sep=",")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kMultipleValues, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.split() got multiple values for argument 'sep'"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();
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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("a,b"));
  AppendExpected(expected_printv_result, PyString::NewInstance("abc"));
  AppendExpected(expected_printv_result, PyString::NewInstance(""));
  AppendExpected(expected_printv_result, PyString::NewInstance("a"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, JoinMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kElementNotStr = R"(
print(",".join(["a", 1]))
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kElementNotStr, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  auto formatted = Runtime_FormatPendingExceptionForStderr();
  std::string message(formatted->buffer(),
                      static_cast<size_t>(formatted->length()));
  EXPECT_NE(message.find("TypeError: sequence item 1: expected str instance"),
            std::string::npos);
  isolate()->exception_state()->Clear();

  constexpr std::string_view kKeyword = R"(
"-".join(iterable=["a", "b"])
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kKeyword, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.join() takes no keyword arguments"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();
}

TEST_F(BasicInterpreterTest, IndexMethodErrors) {
  HandleScope scope;

  constexpr std::string_view kStrNotFound = R"(
"abc".index("d")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kStrNotFound, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("substring not found"), std::string::npos);
  }
  isolate()->exception_state()->Clear();

  constexpr std::string_view kListNotFound = R"(
[1].index(2)
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kListNotFound, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("ValueError"), std::string::npos);
  }
  isolate()->exception_state()->Clear();

  constexpr std::string_view kTupleNotFound = R"(
(1,).index(2)
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kTupleNotFound, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("tuple.index(x): x not in tuple"), std::string::npos);
  }
  isolate()->exception_state()->Clear();

  constexpr std::string_view kStrKeyword = R"(
"abc".index(sub="a")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kStrKeyword, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.index() takes no keyword arguments"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();
}

TEST_F(BasicInterpreterTest, FindAndRfindKeywordErrors) {
  HandleScope scope;

  constexpr std::string_view kFindKeyword = R"(
"abc".find(sub="a")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kFindKeyword, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.find() takes no keyword arguments"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();

  constexpr std::string_view kRfindKeyword = R"(
"abc".rfind(sub="a")
)";
  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kRfindKeyword, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  {
    auto f = Runtime_FormatPendingExceptionForStderr();
    std::string msg(f->buffer(), static_cast<size_t>(f->length()));
    EXPECT_NE(msg.find("str.rfind() takes no keyword arguments"),
              std::string::npos);
  }
  isolate()->exception_state()->Clear();
}

TEST_F(BasicInterpreterTest, ListPop) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = []
i = 0
while i < 5:
  l.append(i)
  i += 1
count = 0
while len(l) > 0:
  count += l.pop()
print(count)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(10)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListReverse) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = []
i = 0
while i < 5:
  l.append(i)
  i += 1

l.reverse()

i = 0

while i < 5:
  print(l[i])
  i += 1
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortInt) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [3, 1, 2]
l.sort()
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance();
  PyList::Append(list, handle(PySmi::FromInt(1)));
  PyList::Append(list, handle(PySmi::FromInt(2)));
  PyList::Append(list, handle(PySmi::FromInt(3)));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortReverse) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [3, 1, 2]
l.sort(reverse=True)
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance();
  PyList::Append(list, handle(PySmi::FromInt(3)));
  PyList::Append(list, handle(PySmi::FromInt(2)));
  PyList::Append(list, handle(PySmi::FromInt(1)));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortKeyLenAndStability) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = ["aa", "b", "ccc", "dd"]
l.sort(key=len)
print(l)

s = ["bb", "aa"]
s.sort(key=len)
print(s)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance();
  PyList::Append(list, PyString::NewInstance("b"));
  PyList::Append(list, PyString::NewInstance("aa"));
  PyList::Append(list, PyString::NewInstance("dd"));
  PyList::Append(list, PyString::NewInstance("ccc"));

  auto stable_list = PyList::NewInstance();
  PyList::Append(stable_list, PyString::NewInstance("bb"));
  PyList::Append(stable_list, PyString::NewInstance("aa"));

  AppendExpected(expected_printv_result, list);
  AppendExpected(expected_printv_result, stable_list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortKey) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [108, 109, 105, 100, 106, 107]
l.sort(key=lambda x : x % 10)
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();

  auto list = PyList::NewInstance();
  PyList::Append(list, handle(PySmi::FromInt(100)));
  PyList::Append(list, handle(PySmi::FromInt(105)));
  PyList::Append(list, handle(PySmi::FromInt(106)));
  PyList::Append(list, handle(PySmi::FromInt(107)));
  PyList::Append(list, handle(PySmi::FromInt(108)));
  PyList::Append(list, handle(PySmi::FromInt(109)));

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortTypeErrorOnMixedTypes) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [1, "a"]
l.sort()
)";

  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kSource, kTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  Handle<PyString> formatted = Runtime_FormatPendingExceptionForStderr();
  std::string message(formatted->buffer(),
                      static_cast<size_t>(formatted->length()));
  EXPECT_NE(message.find("TypeError"), std::string::npos);
  isolate()->exception_state()->Clear();
}

TEST_F(BasicInterpreterTest, ComputeListWithInt) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l1 = ["Hello", "World"]
l2 = [3, 4]
l3 = l1 + l2
l4 = l2 * 3
print(len(l3))
print(len(l4))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListIterator) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [1,2,3,4]
for elem in l:
  print(elem)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildTuple) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = (1,2,3,4)
print(d[2])
print(len(d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
