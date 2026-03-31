// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "src/code/cpython312-pyc-compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/modules/module-manager.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, ImportBuiltinSys) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
import sys
print(sys.version)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "3.12 (saauso mvp)"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, BuiltinModuleMetadataIsStable) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
import sys
import math
import random
import time
print(sys.__name__)
print(math.__name__)
print(random.__name__)
print(time.__name__)
print(sys.__package__)
print(math.__package__)
print(random.__package__)
print(time.__package__)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "sys"));
  AppendExpected(expected, PyString::New(isolate_, "math"));
  AppendExpected(expected, PyString::New(isolate_, "random"));
  AppendExpected(expected, PyString::New(isolate_, "time"));
  AppendExpected(expected, PyString::New(isolate_, ""));
  AppendExpected(expected, PyString::New(isolate_, ""));
  AppendExpected(expected, PyString::New(isolate_, ""));
  AppendExpected(expected, PyString::New(isolate_, ""));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportUserModuleIsCached) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "a_init"));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(1));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportPackageSubmodule) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "pkg_init"));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(42));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportPackageSubmoduleBindsChildOnCacheHit) {
  HandleScope scope(isolate_);

  PyList::Append(isolate()->module_manager()->path(),
                 PyString::New(isolate_, "test/python312/import-mvp"),
                 isolate_);

  Handle<PyString> name = PyString::New(isolate_, "pkg.sub");
  Handle<PyTuple> non_empty_fromlist = PyTuple::New(isolate_, 1);

  MaybeHandle<PyObject> imported_module =
      isolate()->module_manager()->ImportModule(name, non_empty_fromlist, 0,
                                                Handle<PyDict>::null());
  ASSERT_FALSE(imported_module.IsEmpty());

  Handle<PyDict> modules = isolate()->module_manager()->modules();
  Tagged<PyObject> pkg_module_tagged;
  bool found = false;
  ASSERT_TRUE(modules
                  ->GetTagged(PyString::New(isolate_, "pkg"), pkg_module_tagged,
                              isolate_)
                  .To(&found));
  ASSERT_TRUE(found);
  Handle<PyObject> pkg_module = handle(pkg_module_tagged, isolate_);
  ASSERT_FALSE(pkg_module.is_null());

  Handle<PyDict> pkg_dict = PyObject::GetProperties(pkg_module, isolate_);
  ASSERT_FALSE(pkg_dict.is_null());

  Handle<PyString> sub_short_name = PyString::New(isolate_, "sub");
  bool removed = false;
  ASSERT_TRUE(pkg_dict->Remove(sub_short_name, isolate_).To(&removed));
  Tagged<PyObject> bound_tagged;
  ASSERT_TRUE(
      pkg_dict->GetTagged(sub_short_name, bound_tagged, isolate_).To(&found));
  EXPECT_FALSE(found);
  EXPECT_TRUE(bound_tagged.is_null());

  imported_module = isolate()->module_manager()->ImportModule(
      name, non_empty_fromlist, 0, Handle<PyDict>::null());
  ASSERT_FALSE(imported_module.IsEmpty());

  Tagged<PyObject> pkg_sub_module_tagged;
  ASSERT_TRUE(
      modules->GetTagged(name, pkg_sub_module_tagged, isolate_).To(&found));
  ASSERT_TRUE(found);
  Handle<PyObject> pkg_sub_module = handle(pkg_sub_module_tagged, isolate_);
  ASSERT_TRUE(
      pkg_dict->GetTagged(sub_short_name, bound_tagged, isolate_).To(&found));
  ASSERT_TRUE(found);
  Handle<PyObject> bound = handle(bound_tagged, isolate_);
  ASSERT_FALSE(pkg_sub_module.is_null());
  ASSERT_FALSE(bound.is_null());
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, bound, pkg_sub_module).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(BasicInterpreterTest, RelativeImportWorksInsidePackageModule) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "pkg_init"));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(12));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportStarRespectsAllAndSkipsUnderscore) {
  HandleScope scope(isolate_);

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

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(99));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(99));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, ImportPackageSubmoduleUnderGcPressure) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
import sys
sys.path.append("test/python312/import-mvp")

if "pkg.sub" in sys.modules:
    del sys.modules["pkg.sub"]
if "pkg" in sys.modules:
    del sys.modules["pkg"]

hog = []
i = 0
while i < 5000:
    hog.append(str(i))
    i += 1

import pkg.sub
print(pkg.sub.answer)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "pkg_init"));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(42));
  ExpectPrintResult(expected);
}

// 无效模块名（如 "a..b"）会设置 ModuleNotFoundError 并返回空 MaybeHandle，不再
// exit。
TEST_F(BasicInterpreterTest, ImportRejectsInvalidModuleName) {
  HandleScope scope(isolate_);
  Handle<PyString> invalid = PyString::New(isolate_, "a..b");

  MaybeHandle<PyObject> result = isolate()->module_manager()->ImportModule(
      invalid, Handle<PyTuple>::null(), 0, Handle<PyDict>::null());
  ASSERT_TRUE(result.IsEmpty());

  std::string msg = ExpectedAndTakePendingExceptionMessage();
  EXPECT_NE(msg.find("invalid module name"), std::string::npos)
      << "got: " << msg;
}

TEST_F(BasicInterpreterTest, ImportPycModuleWhenSourceMissing) {
  HandleScope scope(isolate_);

  int64_t suffix = static_cast<int64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  std::string suffix_str = std::to_string(suffix);
  std::string module_name = "pyc_only_mod_" + suffix_str;

  std::filesystem::path dir = std::filesystem::temp_directory_path() /
                              ("saauso_import_pyc_" + suffix_str);
  std::filesystem::create_directories(dir);

  std::string file_name = module_name + ".py";
  std::string module_source = "x = 7\nprint(\"pyc_init\")\n";
  std::vector<uint8_t> pyc =
      EmbeddedPython312Compiler::CompileToPycBytes(module_source, file_name);
  ASSERT_FALSE(pyc.empty());

  std::filesystem::path pyc_path = dir / (module_name + ".pyc");
  std::ofstream out(pyc_path, std::ios::binary | std::ios::out);
  out.write(reinterpret_cast<const char*>(pyc.data()),
            static_cast<std::streamsize>(pyc.size()));
  out.close();

  std::string dir_str = dir.generic_string();
  std::string script;
  script += "import sys\n";
  script += "sys.path.append(\"" + dir_str + "\")\n";
  script += "if \"" + module_name + "\" in sys.modules:\n";
  script += "    del sys.modules[\"" + module_name + "\"]\n";
  script += "import " + module_name + "\n";
  script += "print(" + module_name + ".x)\n";

  RunScript(script, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, "pyc_init"));
  AppendExpected(expected, isolate_->factory()->NewSmiFromInt(7));
  ExpectPrintResult(expected);

  std::filesystem::remove_all(dir);
}

}  // namespace saauso::internal
