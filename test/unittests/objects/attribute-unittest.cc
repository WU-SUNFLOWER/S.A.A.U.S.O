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
  HandleScope scope(isolate_);

  auto func_name = PyString::New(isolate_, "foo");
  FunctionTemplateInfo func_template(isolate_, &BUILTIN_FUNC_NAME(DoNoThing),
                                     func_name);

  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithTemplate(func_template);

  auto key = PyString::New(isolate_, "x");
  auto value = isolate_->factory()->NewSmiFromInt(42);

  ASSERT_FALSE(PyObject::SetAttr(isolate_, func, key, value).IsEmpty());
  Handle<PyObject> result;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, func, key).ToHandle(&result));
  EXPECT_TRUE(result.is_identical_to(value));
}

TEST_F(AttributeTest, GetAttrReturnsBoundMethodWithoutCallingIt) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_);
  EXPECT_EQ(list->length(), 0);

  Handle<PyObject> append;
  ASSERT_TRUE(
      PyObject::GetAttr(isolate_, Handle<PyObject>(list),
                        Handle<PyObject>(PyString::New(isolate_, "append")))
          .ToHandle(&append));

  ASSERT_FALSE(append.is_null());
  EXPECT_TRUE(IsMethodObject(append));
  EXPECT_EQ(list->length(), 0);
}

TEST_F(AttributeTest, ClassAccessorReturnsRuntimeTypeObject) {
  HandleScope scope(isolate_);

  auto func_name = PyString::New(isolate_, "foo");
  FunctionTemplateInfo func_template(isolate_, &BUILTIN_FUNC_NAME(DoNoThing),
                                     func_name);

  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithTemplate(func_template);

  Handle<PyObject> func_obj(func);
  Handle<PyObject> expected_class(Handle<PyObject>::cast(
      PyObject::ResolveObjectKlass(func_obj, isolate_)->type_object(isolate_)));
  Handle<PyObject> fake_class = isolate_->factory()->NewSmiFromInt(7);

  Handle<PyDict> properties = PyObject::GetProperties(func_obj, isolate_);
  ASSERT_FALSE(properties.is_null());
  ASSERT_FALSE(
      PyDict::Put(properties, ST(class, isolate_), fake_class, isolate_)
          .IsNothing());

  Handle<PyObject> got_class;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, func_obj, ST(class, isolate_))
                  .ToHandle(&got_class));
  EXPECT_TRUE(got_class.is_identical_to(expected_class));
}

TEST_F(AttributeTest, ClassAccessorRejectsSetAttr) {
  HandleScope scope(isolate_);

  auto func_name = PyString::New(isolate_, "foo");
  FunctionTemplateInfo func_template(isolate_, &BUILTIN_FUNC_NAME(DoNoThing),
                                     func_name);

  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithTemplate(func_template);

  EXPECT_TRUE(PyObject::SetAttr(isolate_, func, ST(class, isolate_),
                                isolate_->factory()->NewSmiFromInt(1))
                  .IsEmpty());
  EXPECT_TRUE(isolate_->HasPendingException());
  isolate_->exception_state()->Clear();
}

TEST_F(AttributeTest, DictAccessorReturnsProperties) {
  HandleScope scope(isolate_);

  auto func_name = PyString::New(isolate_, "foo");
  FunctionTemplateInfo func_template(isolate_, &BUILTIN_FUNC_NAME(DoNoThing),
                                     func_name);

  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithTemplate(func_template);

  Handle<PyObject> func_obj(func);
  Handle<PyDict> properties = PyObject::GetProperties(func_obj, isolate_);
  ASSERT_FALSE(properties.is_null());

  Handle<PyObject> got_dict;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, func_obj, ST(dict, isolate_))
                  .ToHandle(&got_dict));
  EXPECT_TRUE(Handle<PyDict>::cast(got_dict).is_identical_to(properties));
}

TEST_F(AttributeTest, DictAccessorRejectsSetAttr) {
  HandleScope scope(isolate_);

  auto func_name = PyString::New(isolate_, "foo");
  FunctionTemplateInfo func_template(isolate_, &BUILTIN_FUNC_NAME(DoNoThing),
                                     func_name);

  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithTemplate(func_template);

  EXPECT_TRUE(PyObject::SetAttr(isolate_, func, ST(dict, isolate_),
                                Handle<PyObject>(PyDict::New(isolate_)))
                  .IsEmpty());
  EXPECT_TRUE(isolate_->HasPendingException());
  isolate_->exception_state()->Clear();
}

}  // namespace saauso::internal
