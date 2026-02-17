// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, TryExceptCatch) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except ValueError:
  print("caught")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("caught"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptReraise) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  try:
    raise ValueError
  except ValueError:
    print("inner")
    raise
except ValueError:
  print("outer")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("inner"));
  AppendExpected(expected, PyString::NewInstance("outer"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise ValueError
except ValueError:
  print("except")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("except"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, TryExceptFinally2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  raise RuntimeError
except ValueError:
  print("value_error")
except RuntimeError:
  print("runtime eroror")
finally:
  print("finally")
print("after")
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("runtime eroror"));
  AppendExpected(expected, PyString::NewInstance("finally"));
  AppendExpected(expected, PyString::NewInstance("after"));
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal

