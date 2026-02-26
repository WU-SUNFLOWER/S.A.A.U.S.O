// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class BuiltinsBootstrapTest : public VmTestBase {
 protected:
  static void SetUpTestSuite() { VmTestBase::SetUpTestSuite(); }
  static void TearDownTestSuite() { VmTestBase::TearDownTestSuite(); }
};

TEST_F(BuiltinsBootstrapTest, BuiltinsContainCoreEntries) {
  HandleScope scope;

  Handle<PyDict> builtins = isolate_->interpreter()->builtins();

  const char* const kTypeNames[] = {"object", "int",  "str",  "float", "list",
                                    "bool",   "dict", "tuple", "type"};
  for (const char* name : kTypeNames) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsMaybe(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    ASSERT_TRUE(builtins->GetTaggedMaybe(key).To(&value)) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }

  const char* const kOddballs[] = {"True", "False", "None"};
  for (const char* name : kOddballs) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsMaybe(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
  }
  Tagged<PyObject> value;
  ASSERT_TRUE(builtins->GetTaggedMaybe(PyString::NewInstance("True")).To(&value));
  EXPECT_EQ(value, isolate_->py_true_object());
  ASSERT_TRUE(builtins->GetTaggedMaybe(PyString::NewInstance("False")).To(&value));
  EXPECT_EQ(value, isolate_->py_false_object());
  ASSERT_TRUE(builtins->GetTaggedMaybe(PyString::NewInstance("None")).To(&value));
  EXPECT_EQ(value, isolate_->py_none_object());

  const char* const kBuiltinFunctions[] = {"print",      "len",  "isinstance",
                                           "build_class", "sysgc", "exec"};
  for (const char* name : kBuiltinFunctions) {
    Handle<PyString> key = PyString::NewInstance(name);
    if (std::string_view(name) == "build_class") {
      key = ST(func_build_class);
    }
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsMaybe(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    ASSERT_TRUE(builtins->GetTaggedMaybe(key).To(&value)) << name;
    EXPECT_TRUE(IsPyFunction(value)) << name;
  }

  bool exists = false;
  ASSERT_TRUE(builtins->ContainsMaybe(ST(builtins)).To(&exists));
  ASSERT_TRUE(exists);
  ASSERT_TRUE(builtins->GetTaggedMaybe(ST(builtins)).To(&value));
  EXPECT_EQ(value, *builtins);
}

TEST_F(BuiltinsBootstrapTest, BuiltinsContainMvpExceptionTypes) {
  HandleScope scope;

  Handle<PyDict> builtins = isolate_->interpreter()->builtins();

  const char* const kExceptionTypes[] = {
      "BaseException",  "Exception",      "TypeError",    "ValueError",
      "NameError",      "AttributeError", "IndexError",   "KeyError",
      "RuntimeError",
  };
  for (const char* name : kExceptionTypes) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsMaybe(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    ASSERT_TRUE(builtins->GetTaggedMaybe(key).To(&value)) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }
}

}  // namespace saauso::internal
