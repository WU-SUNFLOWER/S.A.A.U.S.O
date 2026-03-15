// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
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

  Handle<PyDict> builtins = handle(isolate_->builtins());

  // 这里的 << 是 Google Test
  // 的流式断言语法，用于在断言失败时输出额外信息（这里是当前遍历到的 name）。
  const char* const kTypeNames[] = {"object", "int",  "str",   "float", "list",
                                    "bool",   "dict", "tuple", "type"};
  for (const char* name : kTypeNames) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(builtins->GetTagged(key, value).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }

  const char* const kOddballs[] = {"True", "False", "None"};
  for (const char* name : kOddballs) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
  }
  Tagged<PyObject> value;
  bool found = false;
  ASSERT_TRUE(
      builtins->GetTagged(PyString::NewInstance("True"), value).To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_true_object());
  ASSERT_TRUE(
      builtins->GetTagged(PyString::NewInstance("False"), value).To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_false_object());
  ASSERT_TRUE(
      builtins->GetTagged(PyString::NewInstance("None"), value).To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_none_object());

  const char* const kBuiltinFunctions[] = {
      "print", "repr", "len", "isinstance", "build_class", "sysgc", "exec"};
  for (const char* name : kBuiltinFunctions) {
    Handle<PyString> key = PyString::NewInstance(name);
    if (std::string_view(name) == "build_class") {
      key = ST(func_build_class);
    }
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    ASSERT_TRUE(builtins->GetTagged(key, value).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyFunction(value)) << name;
  }

  bool exists = false;
  ASSERT_TRUE(builtins->ContainsKey(ST(builtins)).To(&exists));
  ASSERT_TRUE(exists);
  ASSERT_TRUE(builtins->GetTagged(ST(builtins), value).To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, *builtins);
}

TEST_F(BuiltinsBootstrapTest, BuiltinsContainMvpExceptionTypes) {
  HandleScope scope;

  Handle<PyDict> builtins = handle(isolate_->builtins());

  const char* const kExceptionTypes[] = {
      "BaseException",  "Exception",  "TypeError", "ValueError",   "NameError",
      "AttributeError", "IndexError", "KeyError",  "RuntimeError",
  };
  for (const char* name : kExceptionTypes) {
    Handle<PyString> key = PyString::NewInstance(name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(builtins->GetTagged(key, value).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }
}

TEST_F(BuiltinsBootstrapTest, CoreBuiltinTypesExposeReprAndStrMethods) {
  HandleScope scope;
  Handle<PyDict> builtins = handle(isolate_->builtins());

  const char* const kTypeNames[] = {"object", "list", "dict",
                                    "tuple",  "str",  "type"};
  for (const char* name : kTypeNames) {
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(
        builtins->GetTagged(PyString::NewInstance(name), value).To(&found))
        << name;
    ASSERT_TRUE(found) << name;
    ASSERT_TRUE(IsPyTypeObject(value)) << name;

    auto type_obj = Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(value));
    auto props = type_obj->own_klass()->klass_properties();

    bool has_repr = false;
    ASSERT_TRUE(props->ContainsKey(ST(repr)).To(&has_repr)) << name;
    EXPECT_TRUE(has_repr) << name;

    bool has_str = false;
    ASSERT_TRUE(props->ContainsKey(ST(str)).To(&has_str)) << name;
    EXPECT_TRUE(has_str) << name;
  }
}

}  // namespace saauso::internal
