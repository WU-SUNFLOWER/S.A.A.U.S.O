// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, ImportBuiltinTime) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import time

t1 = time.perf_counter()
time.sleep(0.01)
t2 = time.perf_counter()
print(t2 > t1)
print(time.monotonic() >= 0)
print(time.time() > 0)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyTrueObject(isolate_));
  AppendExpected(expected, PyTrueObject(isolate_));
  AppendExpected(expected, PyTrueObject(isolate_));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ListSubclassSurvivesSysgc) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class C(list):
    pass

class D:
    pass

def make():
    c = C()
    d1 = D()
    d1.v = 111
    d2 = D()
    d2.v = 222
    c.append(d1)
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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, handle(PySmi::FromInt(111)));
  AppendExpected(expected, handle(PySmi::FromInt(222)));
  AppendExpected(expected, handle(PySmi::FromInt(111)));
  AppendExpected(expected, handle(PySmi::FromInt(222)));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TimeRejectKeywordArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import time
time.sleep(seconds=0.01)
)";

  RunScriptExpectExceptionContains(
      kSource, "time.sleep() takes no keyword arguments", kTestFileName);
}

}  // namespace saauso::internal
