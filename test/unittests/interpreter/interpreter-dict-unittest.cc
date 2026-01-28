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
  PyDict::Put(expected_dict, handle(PySmi::FromInt(2)),
              PyString::NewInstance("y"));
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

}  // namespace saauso::internal

