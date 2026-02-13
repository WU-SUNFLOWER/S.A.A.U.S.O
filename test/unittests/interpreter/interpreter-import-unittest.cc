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

TEST_F(BasicInterpreterTest, ImportBuiltinSys) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import sys
print(sys.version)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("3.12 (saauso mvp)"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportUserModuleIsCached) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import sys
sys.path.append("test/python312/import-mvp")

if "a" in sys.modules:
    del sys.modules["a"]

import a
import a
print(a.x)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("a_init"));
  AppendExpected(expected, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportPackageSubmodule) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import sys
sys.path.append("test/python312/import-mvp")

if "pkg.sub" in sys.modules:
    del sys.modules["pkg.sub"]
if "pkg" in sys.modules:
    del sys.modules["pkg"]

import pkg.sub
print(pkg.sub.answer)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("pkg_init"));
  AppendExpected(expected, handle(PySmi::FromInt(42)));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, RelativeImportWorksInsidePackageModule) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import sys
sys.path.append("test/python312/import-mvp")

if "pkg.rel.mod" in sys.modules:
    del sys.modules["pkg.rel.mod"]
if "pkg.rel.helper" in sys.modules:
    del sys.modules["pkg.rel.helper"]
if "pkg.rel" in sys.modules:
    del sys.modules["pkg.rel"]
if "pkg.x" in sys.modules:
    del sys.modules["pkg.x"]
if "pkg" in sys.modules:
    del sys.modules["pkg"]

import pkg.rel.mod
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyString::NewInstance("pkg_init"));
  AppendExpected(expected, handle(PySmi::FromInt(12)));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportStarRespectsAllAndSkipsUnderscore) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import sys
sys.path.append("test/python312/import-mvp")

if "star" in sys.modules:
    del sys.modules["star"]

c = 99
_b = 99
from star import *
print(a)
print(c)
print(_b)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, handle(PySmi::FromInt(1)));
  AppendExpected(expected, handle(PySmi::FromInt(99)));
  AppendExpected(expected, handle(PySmi::FromInt(99)));
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal
