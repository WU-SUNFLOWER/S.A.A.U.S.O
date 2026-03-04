// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/templates.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class AttributeTest : public VmTestBase {};

BUILTIN(DoNoThing) {
  return Handle<PyObject>::null();
}

TEST_F(AttributeTest, DefaultSetAttrCreatesPropertiesDict) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  FunctionTemplateInfo func_template(&BUILTIN_FUNC_NAME(DoNoThing), func_name);

  Handle<PyFunction> func;
  ASSERT_TRUE(
      isolate_->factory()->NewPyFunctionWithTemplate(func_template).To(&func));

  auto key = PyString::NewInstance("x");
  auto value = handle(PySmi::FromInt(42));

  ASSERT_FALSE(PyObject::SetAttr(func, key, value).IsEmpty());
  Handle<PyObject> result;
  ASSERT_TRUE(PyObject::GetAttr(func, key).ToHandle(&result));
  EXPECT_TRUE(result.is_identical_to(value));
}

TEST_F(AttributeTest, GetAttrReturnsBoundMethodWithoutCallingIt) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  EXPECT_EQ(list->length(), 0);

  Handle<PyObject> append;
  ASSERT_TRUE(
      PyObject::GetAttr(Handle<PyObject>(list),
                        Handle<PyObject>(PyString::NewInstance("append")))
          .ToHandle(&append));

  ASSERT_FALSE(append.is_null());
  EXPECT_TRUE(IsMethodObject(append));
  EXPECT_EQ(list->length(), 0);
}

}  // namespace saauso::internal
