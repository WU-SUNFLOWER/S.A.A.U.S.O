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

TEST_F(BasicInterpreterTest, SubclassList) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(list):
    pass

i = 0
last = None
while i < 2000:
    c = C()
    c.append(i)
    c.x = i
    last = c
    i = i + 1

print(len(last))
print(last[0])
print(last.x)
print(1 if last else 0)
last.pop()
print(1 if last else 0)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1999)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1999)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassListCustomInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(list):
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def foo(self):
        return self

c = C(1, 2, 3)
print(c.foo().x)
print(c.foo().y)
print(c.foo().z)
print(len(c.foo()))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
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

TEST_F(BasicInterpreterTest, ListIndexPropagatesExceptionFromEq) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class X:
  def __eq__(self, other):
    raise RuntimeError("boom")

l = [X()]
l.index(X())
)";

  RunScriptExpectExceptionContains(kSource, "RuntimeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, TupleIndexPropagatesExceptionFromEq) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class X:
  def __eq__(self, other):
    raise RuntimeError("boom")

t = (X(),)
t.index(X())
)";

  RunScriptExpectExceptionContains(kSource, "RuntimeError", kTestFileName);
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
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
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

TEST_F(BasicInterpreterTest, SubclassTuple) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(tuple):
  pass

c = C()
c.x = 7
print(len(c))
print(c.x)

c2 = C((1, 2, 3))
print(len(c2))
print(c2[1])
print(1 if (2 in c2) else 0)
print(c2.index(2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassTupleSurvivesSysgc) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(tuple):
  pass

class D:
  pass

def make():
  d1 = D()
  d1.v = 111
  d2 = D()
  d2.v = 222
  c = C((d1,))
  c.x = d2
  return c

c = make()

tmp = []
i = 0
while i < 5000:
  tmp.append([i, i + 1, i + 2])
  i = i + 1

sysgc()
print(c[0].v)
print(c.x.v)

sysgc()
print(c[0].v)
print(c.x.v)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(111)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(222)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(111)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(222)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassTupleCustomInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(tuple):
  def __init__(self, x, y):
    self.x = x
    self.y = y

  def foo(self):
    return self

c = C(1, 2)
print(c.foo().x)
print(c.foo().y)
print(len(c.foo()))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStr) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class S(str):
  pass

s = S()
s.x = 7
print(len(s))
print(s.x)

s2 = S("hi")
print(len(s2))
print(s2)
print(s2.upper())
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, PyString::NewInstance("hi"));
  AppendExpected(expected_printv_result, PyString::NewInstance("HI"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStrSurvivesSysgc) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class S(str):
  pass

class D:
  pass

def make():
  d = D()
  d.v = 123
  s = S("hello")
  s.x = d
  return s

s = make()

tmp = []
i = 0
while i < 5000:
  tmp.append([i, i + 1, i + 2])
  i = i + 1

sysgc()
print(s.x.v)
print(s)

sysgc()
print(s.x.v)
print(s)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(123)));
  AppendExpected(expected_printv_result, PyString::NewInstance("hello"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(123)));
  AppendExpected(expected_printv_result, PyString::NewInstance("hello"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassStrCustomInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class S(str):
  def __init__(self, x):
    self.x = x

  def foo(self):
    return self

s = S(7)
print(s.foo().x)
print(len(s.foo()))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(0)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
