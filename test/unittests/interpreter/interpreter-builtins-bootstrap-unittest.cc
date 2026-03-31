// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception-klass.h"
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

  Handle<PyDict> builtins = isolate_->builtins();

  // 这里的 << 是 Google Test
  // 的流式断言语法，用于在断言失败时输出额外信息（这里是当前遍历到的 name）。
  const char* const kTypeNames[] = {"object", "int",  "str",   "float", "list",
                                    "bool",   "dict", "tuple", "type"};
  for (const char* name : kTypeNames) {
    Handle<PyString> key = PyString::New(isolate_, name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key, isolate_).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(builtins->GetTagged(key, value, isolate_).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }

  const char* const kOddballs[] = {"True", "False", "None"};
  for (const char* name : kOddballs) {
    Handle<PyString> key = PyString::New(isolate_, name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key, isolate_).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
  }
  Tagged<PyObject> value;
  bool found = false;
  ASSERT_TRUE(builtins->GetTagged(ST(true_symbol, isolate_), value, isolate_)
                  .To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_true_object());
  ASSERT_TRUE(builtins->GetTagged(ST(false_symbol, isolate_), value, isolate_)
                  .To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_false_object());
  ASSERT_TRUE(builtins->GetTagged(ST(none_symbol, isolate_), value, isolate_)
                  .To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, isolate_->py_none_object());

  const char* const kBuiltinFunctions[] = {
      "print", "repr", "len", "isinstance", "build_class", "sysgc", "exec"};
  for (const char* name : kBuiltinFunctions) {
    Handle<PyString> key = PyString::New(isolate_, name);
    if (std::string_view(name) == "build_class") {
      key = ST(func_build_class, isolate_);
    }
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key, isolate_).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    ASSERT_TRUE(builtins->GetTagged(key, value, isolate_).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyFunction(value, isolate_)) << name;
  }

  bool exists = false;
  ASSERT_TRUE(
      builtins->ContainsKey(ST(builtins, isolate_), isolate_).To(&exists));
  ASSERT_TRUE(exists);
  ASSERT_TRUE(
      builtins->GetTagged(ST(builtins, isolate_), value, isolate_).To(&found));
  ASSERT_TRUE(found);
  EXPECT_EQ(value, *builtins);
}

TEST_F(BuiltinsBootstrapTest, BuiltinsContainMvpExceptionTypes) {
  HandleScope scope;

  Handle<PyDict> builtins = isolate_->builtins();

  const char* const kExceptionTypes[] = {
      "BaseException",  "Exception",  "TypeError", "ValueError",   "NameError",
      "AttributeError", "IndexError", "KeyError",  "RuntimeError",
  };
  // 第一层校验：
  // 1) 这些 MVP 异常类型都已注册到 builtins；
  // 2) 每个条目都是 type object；
  // 3) BaseException 的 type object 必须绑定到独立的 PyBaseExceptionKlass。
  for (const char* name : kExceptionTypes) {
    Handle<PyString> key = PyString::New(isolate_, name);
    bool exists = false;
    ASSERT_TRUE(builtins->ContainsKey(key, isolate_).To(&exists)) << name;
    ASSERT_TRUE(exists) << name;
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(builtins->GetTagged(key, value, isolate_).To(&found)) << name;
    ASSERT_TRUE(found) << name;
    EXPECT_TRUE(IsPyTypeObject(value)) << name;
  }

  Tagged<PyObject> base_exception_value;
  bool found = false;
  ASSERT_TRUE(builtins
                  ->GetTagged(PyString::New(isolate_, "BaseException"),
                              base_exception_value, isolate_)
                  .To(&found));
  ASSERT_TRUE(found);

  // 第二层校验：校验 BaseException 的 klass 指向正确
  auto base_exception_type = Tagged<PyTypeObject>::cast(base_exception_value);
  auto base_exception_klass = base_exception_type->own_klass();
  EXPECT_EQ(base_exception_klass, PyBaseExceptionKlass::GetInstance(isolate_));

  // 第三层校验：
  // 直接检查 BaseException 的类字典，确认关键魔法方法入口已存在。
  // 这里验证的是“类型对象安装正确”，而不是方法行为语义本身。
  auto base_exception_props = base_exception_klass->klass_properties(isolate_);

  bool has_init = false;
  ASSERT_TRUE(
      base_exception_props->ContainsKey(ST(init_instance, isolate_), isolate_)
          .To(&has_init));
  EXPECT_TRUE(has_init);

  bool has_repr = false;
  ASSERT_TRUE(base_exception_props->ContainsKey(ST(repr, isolate_), isolate_)
                  .To(&has_repr));
  EXPECT_TRUE(has_repr);

  bool has_str = false;
  ASSERT_TRUE(base_exception_props->ContainsKey(ST(str, isolate_), isolate_)
                  .To(&has_str));
  EXPECT_TRUE(has_str);
}

TEST_F(BuiltinsBootstrapTest, CoreBuiltinTypesExposeReprAndStrMethods) {
  HandleScope scope;
  Handle<PyDict> builtins = isolate_->builtins();

  const char* const kTypeNames[] = {"object", "list", "dict",
                                    "tuple",  "str",  "type"};
  for (const char* name : kTypeNames) {
    Tagged<PyObject> value;
    bool found = false;
    ASSERT_TRUE(
        builtins->GetTagged(PyString::New(isolate_, name), value, isolate_)
            .To(&found))
        << name;
    ASSERT_TRUE(found) << name;
    ASSERT_TRUE(IsPyTypeObject(value)) << name;

    auto type_obj = Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(value));
    auto props = type_obj->own_klass()->klass_properties(isolate_);

    bool has_repr = false;
    ASSERT_TRUE(props->ContainsKey(ST(repr, isolate_), isolate_).To(&has_repr))
        << name;
    EXPECT_TRUE(has_repr) << name;

    bool has_str = false;
    ASSERT_TRUE(props->ContainsKey(ST(str, isolate_), isolate_).To(&has_str))
        << name;
    EXPECT_TRUE(has_str) << name;
  }
}

}  // namespace saauso::internal
