// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/cell.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/templates.h"
#include "src/runtime/runtime-reflection.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyObjectTest : public VmTestBase {};

namespace {

MaybeHandle<PyObject> DummyMethodTarget(Isolate*,
                                        Handle<PyObject>,
                                        Handle<PyTuple>,
                                        Handle<PyDict>) {
  return Handle<PyObject>::null();
}

}  // namespace

TEST_F(PyObjectTest, GetPropertiesReturnsNullByDefaultAndAcceptsNullSet) {
  HandleScope scope;

  auto list = PyList::New(isolate_);
  auto props = PyObject::GetProperties(Handle<PyObject>(list), isolate_);
  EXPECT_TRUE(props.is_null());

  PyObject::SetProperties(*Handle<PyObject>(list), Tagged<PyDict>::null());
  auto props_after = PyObject::GetProperties(Handle<PyObject>(list), isolate_);
  EXPECT_TRUE(props_after.is_null());
}

TEST_F(PyObjectTest, SetPropertiesStoresDictAndGetPropertiesReturnsIt) {
  HandleScope scope;

  auto list = PyList::New(isolate_);
  auto dict = PyDict::New(isolate_);

  PyObject::SetProperties(*Handle<PyObject>(list), *dict);
  auto props = PyObject::GetProperties(Handle<PyObject>(list), isolate_);

  ASSERT_FALSE(props.is_null());
  EXPECT_TRUE(props.is_identical_to(dict));
}

TEST_F(PyObjectTest, IsHeapObjectReturnsFalseForNullAndSmi) {
  HandleScope scope;

  Tagged<PyObject> null = Tagged<PyObject>::null();
  EXPECT_FALSE(IsHeapObject(null));

  Tagged<PyObject> smi = Tagged<PyObject>(PySmi::FromInt(123));
  EXPECT_FALSE(IsHeapObject(smi));

  auto list = PyList::New(isolate_);
  EXPECT_TRUE(IsHeapObject(*Handle<PyObject>(list)));
}

TEST_F(PyObjectTest, IsPyContainerAndStringSupportLikeAndExactSemantics) {
  HandleScope scope;
  Handle<PyObject> exact_list(PyList::New(isolate_));
  Handle<PyObject> exact_dict(PyDict::New(isolate_));
  Handle<PyObject> exact_tuple(PyTuple::New(isolate_, 0));
  Handle<PyObject> exact_string(PyString::New(isolate_, "exact"));

  EXPECT_TRUE(IsPyList(exact_list));
  EXPECT_TRUE(IsPyListExact(exact_list, isolate_));
  EXPECT_FALSE(IsPyDict(exact_list));
  EXPECT_FALSE(IsPyTuple(exact_list));
  EXPECT_FALSE(IsPyString(exact_list));

  EXPECT_TRUE(IsPyDict(exact_dict));
  EXPECT_TRUE(IsPyDictExact(exact_dict, isolate_));
  EXPECT_FALSE(IsPyList(exact_dict));
  EXPECT_FALSE(IsPyTuple(exact_dict));
  EXPECT_FALSE(IsPyString(exact_dict));

  EXPECT_TRUE(IsPyTuple(exact_tuple));
  EXPECT_TRUE(IsPyTupleExact(exact_tuple, isolate_));
  EXPECT_FALSE(IsPyList(exact_tuple));
  EXPECT_FALSE(IsPyDict(exact_tuple));
  EXPECT_FALSE(IsPyString(exact_tuple));

  EXPECT_TRUE(IsPyString(exact_string));
  EXPECT_TRUE(IsPyStringExact(exact_string, isolate_));
  EXPECT_FALSE(IsPyList(exact_string));
  EXPECT_FALSE(IsPyDict(exact_string));
  EXPECT_FALSE(IsPyTuple(exact_string));

  Handle<PyString> list_like_name = PyString::New(isolate_, "ListLikeCase");
  Handle<PyDict> list_like_props = PyDict::New(isolate_);
  Handle<PyList> list_like_supers = PyList::New(isolate_, 1);
  list_like_supers->SetAndExtendLength(
      0, PyListKlass::GetInstance(isolate_)->type_object(isolate_));
  Handle<PyTypeObject> list_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, list_like_name,
                                        list_like_props, list_like_supers)
                  .ToHandle(&list_like_type));
  Handle<PyObject> list_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, list_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&list_like));
  EXPECT_TRUE(IsPyList(list_like));
  EXPECT_FALSE(IsPyListExact(list_like, isolate_));
  EXPECT_FALSE(IsPyDict(list_like));
  EXPECT_FALSE(IsPyTuple(list_like));
  EXPECT_FALSE(IsPyString(list_like));
  EXPECT_FALSE(PyObject::GetProperties(list_like, isolate_).is_null());

  Handle<PyString> dict_like_name = PyString::New(isolate_, "DictLikeCase");
  Handle<PyDict> dict_like_props = PyDict::New(isolate_);
  Handle<PyList> dict_like_supers = PyList::New(isolate_, 1);
  dict_like_supers->SetAndExtendLength(
      0, PyDictKlass::GetInstance(isolate_)->type_object(isolate_));
  Handle<PyTypeObject> dict_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, dict_like_name,
                                        dict_like_props, dict_like_supers)
                  .ToHandle(&dict_like_type));
  Handle<PyObject> dict_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, dict_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&dict_like));
  EXPECT_TRUE(IsPyDict(dict_like));
  EXPECT_FALSE(IsPyDictExact(dict_like, isolate_));
  EXPECT_FALSE(IsPyList(dict_like));
  EXPECT_FALSE(IsPyTuple(dict_like));
  EXPECT_FALSE(IsPyString(dict_like));
  EXPECT_FALSE(PyObject::GetProperties(dict_like, isolate_).is_null());

  Handle<PyString> tuple_like_name = PyString::New(isolate_, "TupleLikeCase");
  Handle<PyDict> tuple_like_props = PyDict::New(isolate_);
  Handle<PyList> tuple_like_supers = PyList::New(isolate_, 1);
  tuple_like_supers->SetAndExtendLength(
      0, PyTupleKlass::GetInstance(isolate_)->type_object(isolate_));
  Handle<PyTypeObject> tuple_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, tuple_like_name,
                                        tuple_like_props, tuple_like_supers)
                  .ToHandle(&tuple_like_type));
  Handle<PyObject> tuple_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, tuple_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&tuple_like));
  EXPECT_TRUE(IsPyTuple(tuple_like));
  EXPECT_FALSE(IsPyTupleExact(tuple_like, isolate_));
  EXPECT_FALSE(IsPyList(tuple_like));
  EXPECT_FALSE(IsPyDict(tuple_like));
  EXPECT_FALSE(IsPyString(tuple_like));
  EXPECT_FALSE(PyObject::GetProperties(tuple_like, isolate_).is_null());

  Handle<PyString> string_like_name = PyString::New(isolate_, "StringLikeCase");
  Handle<PyDict> string_like_props = PyDict::New(isolate_);
  Handle<PyList> string_like_supers = PyList::New(isolate_, 1);
  string_like_supers->SetAndExtendLength(
      0, PyStringKlass::GetInstance(isolate_)->type_object(isolate_));
  Handle<PyTypeObject> string_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, string_like_name,
                                        string_like_props, string_like_supers)
                  .ToHandle(&string_like_type));
  Handle<PyObject> string_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, string_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&string_like));
  EXPECT_TRUE(IsPyString(string_like));
  EXPECT_FALSE(IsPyStringExact(string_like, isolate_));
  EXPECT_FALSE(IsPyList(string_like));
  EXPECT_FALSE(IsPyDict(string_like));
  EXPECT_FALSE(IsPyTuple(string_like));
  EXPECT_FALSE(PyObject::GetProperties(string_like, isolate_).is_null());
}

TEST_F(PyObjectTest, IsExactByKindBuiltinCheckersRemainPrecise) {
  HandleScope scope;

  Handle<PyObject> type_object(
      PyListKlass::GetInstance(isolate_)->type_object(isolate_));
  Handle<PyObject> py_float(PyFloat::New(isolate_, 3.14));
  Handle<PyObject> code_object(isolate_->factory()->NewPyCodeObject());
  Handle<PyObject> fixed_array(isolate_->factory()->NewFixedArray(2));
  Handle<PyObject> cell(isolate_->factory()->NewCell());

  Handle<PyList> list = PyList::New(isolate_);
  Handle<PyTuple> tuple = PyTuple::New(isolate_, 0);
  Handle<PyDict> dict = PyDict::New(isolate_);
  Handle<PyObject> list_iterator(isolate_->factory()->NewPyListIterator(list));
  Handle<PyObject> tuple_iterator(
      isolate_->factory()->NewPyTupleIterator(Handle<PyObject>(tuple)));
  Handle<PyObject> dict_keys(isolate_->factory()->NewPyDictKeys(dict));
  Handle<PyObject> dict_values(isolate_->factory()->NewPyDictValues(dict));
  Handle<PyObject> dict_items(isolate_->factory()->NewPyDictItems(dict));
  Handle<PyObject> dict_key_iterator(
      isolate_->factory()->NewPyDictKeyIterator(dict));
  Handle<PyObject> dict_value_iterator(
      isolate_->factory()->NewPyDictValueIterator(dict));
  Handle<PyObject> dict_item_iterator(
      isolate_->factory()->NewPyDictItemIterator(dict));

  Handle<PyFunction> func;
  FunctionTemplateInfo func_template(isolate_, &DummyMethodTarget,
                                     PyString::New(isolate_, "dummy"));
  ASSERT_TRUE(isolate_->factory()
                  ->NewPyFunctionWithTemplate(func_template)
                  .ToHandle(&func));
  Handle<PyObject> method(isolate_->factory()->NewMethodObject(func, list));

  EXPECT_TRUE(IsPyTypeObject(type_object));
  EXPECT_FALSE(IsPyFloat(type_object));
  EXPECT_FALSE(IsPyCodeObject(type_object));

  EXPECT_TRUE(IsPyFloat(py_float));
  EXPECT_FALSE(IsPyTypeObject(py_float));
  EXPECT_FALSE(IsFixedArray(py_float));

  EXPECT_TRUE(IsPyCodeObject(code_object));
  EXPECT_FALSE(IsPyFloat(code_object));
  EXPECT_FALSE(IsCell(code_object));

  EXPECT_TRUE(IsFixedArray(fixed_array));
  EXPECT_FALSE(IsCell(fixed_array));
  EXPECT_FALSE(IsMethodObject(fixed_array));

  EXPECT_TRUE(IsCell(cell));
  EXPECT_FALSE(IsFixedArray(cell));
  EXPECT_FALSE(IsMethodObject(cell));

  EXPECT_TRUE(IsMethodObject(method));
  EXPECT_FALSE(IsPyListIterator(method));
  EXPECT_FALSE(IsPyTupleIterator(method));

  EXPECT_TRUE(IsPyListIterator(list_iterator));
  EXPECT_FALSE(IsPyTupleIterator(list_iterator));
  EXPECT_FALSE(IsPyDictKeyIterator(list_iterator));

  EXPECT_TRUE(IsPyTupleIterator(tuple_iterator));
  EXPECT_FALSE(IsPyListIterator(tuple_iterator));
  EXPECT_FALSE(IsPyDictValueIterator(tuple_iterator));

  EXPECT_TRUE(IsPyDictKeys(dict_keys));
  EXPECT_FALSE(IsPyDictValues(dict_keys));
  EXPECT_FALSE(IsPyDictItems(dict_keys));

  EXPECT_TRUE(IsPyDictValues(dict_values));
  EXPECT_FALSE(IsPyDictKeys(dict_values));
  EXPECT_FALSE(IsPyDictItems(dict_values));

  EXPECT_TRUE(IsPyDictItems(dict_items));
  EXPECT_FALSE(IsPyDictKeys(dict_items));
  EXPECT_FALSE(IsPyDictValues(dict_items));

  EXPECT_TRUE(IsPyDictKeyIterator(dict_key_iterator));
  EXPECT_FALSE(IsPyDictItemIterator(dict_key_iterator));
  EXPECT_FALSE(IsPyDictValueIterator(dict_key_iterator));

  EXPECT_TRUE(IsPyDictValueIterator(dict_value_iterator));
  EXPECT_FALSE(IsPyDictKeyIterator(dict_value_iterator));
  EXPECT_FALSE(IsPyDictItemIterator(dict_value_iterator));

  EXPECT_TRUE(IsPyDictItemIterator(dict_item_iterator));
  EXPECT_FALSE(IsPyDictKeyIterator(dict_item_iterator));
  EXPECT_FALSE(IsPyDictValueIterator(dict_item_iterator));
}

}  // namespace saauso::internal
