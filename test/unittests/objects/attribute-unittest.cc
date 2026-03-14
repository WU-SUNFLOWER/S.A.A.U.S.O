// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/templates.h"
#include "src/runtime/string-table.h"
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

TEST_F(AttributeTest, ClassAccessorReturnsRuntimeTypeObject) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  FunctionTemplateInfo func_template(&BUILTIN_FUNC_NAME(DoNoThing), func_name);

  Handle<PyFunction> func;
  ASSERT_TRUE(
      isolate_->factory()->NewPyFunctionWithTemplate(func_template).To(&func));

  Handle<PyObject> func_obj(func);
  Handle<PyObject> expected_class(
      Handle<PyObject>::cast(PyObject::GetKlass(func_obj)->type_object()));
  Handle<PyObject> fake_class = handle(PySmi::FromInt(7));

  Handle<PyDict> properties = PyObject::GetProperties(func_obj);
  ASSERT_FALSE(properties.is_null());
  ASSERT_FALSE(PyDict::Put(properties, ST(class), fake_class).IsNothing());

  Handle<PyObject> got_class;
  ASSERT_TRUE(PyObject::GetAttr(func_obj, ST(class)).ToHandle(&got_class));
  EXPECT_TRUE(got_class.is_identical_to(expected_class));
}

TEST_F(AttributeTest, ClassAccessorRejectsSetAttr) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  FunctionTemplateInfo func_template(&BUILTIN_FUNC_NAME(DoNoThing), func_name);

  Handle<PyFunction> func;
  ASSERT_TRUE(
      isolate_->factory()->NewPyFunctionWithTemplate(func_template).To(&func));

  EXPECT_TRUE(
      PyObject::SetAttr(func, ST(class), handle(PySmi::FromInt(1))).IsEmpty());
  EXPECT_TRUE(isolate_->HasPendingException());
  isolate_->exception_state()->Clear();
}

TEST_F(AttributeTest, DictAccessorReturnsProperties) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  FunctionTemplateInfo func_template(&BUILTIN_FUNC_NAME(DoNoThing), func_name);

  Handle<PyFunction> func;
  ASSERT_TRUE(
      isolate_->factory()->NewPyFunctionWithTemplate(func_template).To(&func));

  Handle<PyObject> func_obj(func);
  Handle<PyDict> properties = PyObject::GetProperties(func_obj);
  ASSERT_FALSE(properties.is_null());

  Handle<PyObject> got_dict;
  ASSERT_TRUE(PyObject::GetAttr(func_obj, ST(dict)).ToHandle(&got_dict));
  EXPECT_TRUE(Handle<PyDict>::cast(got_dict).is_identical_to(properties));
}

TEST_F(AttributeTest, DictAccessorRejectsSetAttr) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  FunctionTemplateInfo func_template(&BUILTIN_FUNC_NAME(DoNoThing), func_name);

  Handle<PyFunction> func;
  ASSERT_TRUE(
      isolate_->factory()->NewPyFunctionWithTemplate(func_template).To(&func));

  EXPECT_TRUE(
      PyObject::SetAttr(func, ST(dict), Handle<PyObject>(PyDict::NewInstance()))
          .IsEmpty());
  EXPECT_TRUE(isolate_->HasPendingException());
  isolate_->exception_state()->Clear();
}

}  // namespace saauso::internal
