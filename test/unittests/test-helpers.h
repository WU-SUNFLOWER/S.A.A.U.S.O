// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef TEST_UNITTESTS_TEST_HELPERS_H_
#define TEST_UNITTESTS_TEST_HELPERS_H_

#include <string_view>

#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

template <typename T>
class Global;

template <typename T>
class Handle;

class Isolate;
class PyDict;
class PyList;
class PyObject;
class PyTuple;

class VmTestBase : public ::testing::Test {
 protected:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

  static Isolate* isolate_;
};

class IsolateOnlyTestBase : public ::testing::Test {
 protected:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

  static Isolate* isolate_;
};

class EmbeddedPython312VmTestBase : public VmTestBase {
 protected:
  static void TearDownTestSuite();
};

class BasicInterpreterTest : public EmbeddedPython312VmTestBase {
 protected:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

  void SetUp() override;

  static void RunScript(std::string_view source, std::string_view file_name);

  static void ExpectPrintResult(Handle<PyList> expected);

  static Isolate* isolate();

 private:
  static Handle<PyObject> Native_PrintV(Handle<PyObject> host,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs);

  static Global<PyList> printv_result_;
};

inline constexpr std::string_view kInterpreterTestFileName = "saauso_test.py";

}  // namespace saauso::internal

#endif  // TEST_UNITTESTS_TEST_HELPERS_H_
