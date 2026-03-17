// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/objects/py-float.h"
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

}  // namespace saauso::internal
