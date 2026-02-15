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

TEST_F(BasicInterpreterTest, ImportBuiltinRandom) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import random

random.seed(1)
a = random.random()
random.seed(1)
b = random.random()
print(a == b)

print(random.randint(5, 5))

v = random.randrange(0, 10, 2)
print(v % 2 == 0)

print(random.getrandbits(8) >= 0)

xs = [1, 2, 3, 4]
random.seed(2)
random.shuffle(xs)
ys = [1, 2, 3, 4]
random.seed(2)
random.shuffle(ys)
print(xs == ys)

print(random.choice((7, 8, 9)) in (7, 8, 9))
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, handle(PySmi::FromInt(5)));
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, PyTrueObject());
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal

