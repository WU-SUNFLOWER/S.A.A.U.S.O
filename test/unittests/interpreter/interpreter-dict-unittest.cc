// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-dict.h"
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

TEST_F(BasicInterpreterTest, BuildDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {1 : "hello", "world" : 2}
print(d[1])
print(d["world"])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("hello"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildDict2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
a = 1
b = 2
d = {a : "hello", b : "world"}
print(d[a])
print(d[b])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("hello"));
  AppendExpected(expected_printv_result, PyString::NewInstance("world"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, StoreValueToDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = "hello"
y = "world"
d = {}
d[x] = y
print(d[x])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("world"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictSetDefaultMethod) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
print(d.setdefault(1))

d = {1: "x"}
print(d.setdefault(1, 2))

d = {}
v = [1]
r = d.setdefault("k", v)
print(r is v)
print(d["k"] is v)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyNoneObject());
  AppendExpected(expected_printv_result, PyString::NewInstance("x"));
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictPopMethod) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {1: "x", 2: "y"}
print(d.pop(1))
print(d)
print(d.pop(42, "z"))
print(d)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("x"));

  auto expected_dict = PyDict::NewInstance();
  ASSERT_FALSE(PyDict::Put(expected_dict, handle(PySmi::FromInt(2)),
                           PyString::NewInstance("y"))
                   .IsNothing());
  AppendExpected(expected_printv_result, expected_dict);

  AppendExpected(expected_printv_result, PyString::NewInstance("z"));
  AppendExpected(expected_printv_result, expected_dict);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictIterator) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {"x": 1, "y": 2, "z": 3}
sum = 0
for key in d:
  sum += d[key]
print(sum)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictKeysAndValuesView) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {"x": 1, "y": 2, "z": 3}
print(len(d.keys()))
print(len(d.values()))

sum = 0
for v in d.values():
  sum += v
print(sum)

cnt = 0
for k in d.keys():
  cnt += 1
print(cnt)

print("x" in d.keys())
print(2 in d.values())
print(42 in d.values())
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictItemsView) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {"x": 1, "y": 2, "z": 3}
print(len(d.items()))

sum = 0
for it in d.items():
  sum += it[1]
print(sum)

print(("x", 1) in d.items())
print(("x", 2) in d.items())
print(("q", 1) in d.items())
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(6)));
  AppendExpected(expected_printv_result, PyTrueObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  AppendExpected(expected_printv_result, PyFalseObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictConstructorKwargs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = dict(name="Alice", age=30, city="Beijing")
print(d["name"])
print(d["age"])
print(d["city"])
print(len(d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyString::NewInstance("Alice"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(30)));
  AppendExpected(expected_printv_result, PyString::NewInstance("Beijing"));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictConstructorListOfTuples) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = dict([("one", 1), ("two", 2), ("three", 3)])
print(d["one"])
print(d["two"])
print(d["three"])
print(len(d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictConstructorCopyFromDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
old_dict = {"x": 10, "y": 20}
new_dict = dict(old_dict)
print(new_dict is old_dict)
print(new_dict["x"])
print(new_dict["y"])
print(len(new_dict))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyFalseObject());
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(10)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(20)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictConstructorIterableThenKwOverride) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = dict([("a", 1), ("b", 2)], b=3, c=4)
print(d["a"])
print(d["b"])
print(d["c"])
print(len(d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(4)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictGetItemUnhashableKeyPropagates) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
  pass

d = {}
print(d[A()])
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: unhashable type: 'A'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, DictSetDefaultUnhashableKeyPropagates) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
  pass

d = {}
print(d.setdefault(A(), 1))
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: unhashable type: 'A'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, DictGetMethodUnhashableKeyPropagates) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
  pass

d = {}
print(d.get(A()))
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: unhashable type: 'A'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, DictKeysContainsUnhashableKeyPropagates) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
  pass

d = {}
print(A() in d.keys())
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: unhashable type: 'A'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, DictItemsContainsUnhashableKeyPropagates) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class A:
  pass

d = {}
print((A(), 1) in d.items())
)";

  RunScriptExpectExceptionContains(kSource, "TypeError: unhashable type: 'A'",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, DictGetMethodDefaultUsedOnMiss) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
print(d.get("k", 1))
print(d.get("k"))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, PyNoneObject());
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictSetDefaultDefaultUsedOnInsert) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
print(d.setdefault("k", 42))
print(d["k"])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(42)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(42)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DictPopDefaultUsedOnMiss) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
print(d.pop("k", 7))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(dict):
  pass

c = C()
c["a"] = 1
c["b"] = 2
c.x = 3
print(len(c))
print(c["a"])
print(c["b"])
print(c.x)
print(1 if ("a" in c) else 0)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SubclassDictSurvivesSysgc) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(dict):
  pass

class D:
  pass

def make():
  c = C()
  d1 = D()
  d1.v = 111
  d2 = D()
  d2.v = 222
  c["k"] = d1
  c.x = d2
  return c

c = make()

tmp = []
i = 0
while i < 5000:
  tmp.append([i, i + 1, i + 2])
  i = i + 1

sysgc()
print(c["k"].v)
print(c.x.v)

sysgc()
print(c["k"].v)
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

TEST_F(BasicInterpreterTest, SubclassDictCustomInit) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(dict):
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

}  // namespace saauso::internal
