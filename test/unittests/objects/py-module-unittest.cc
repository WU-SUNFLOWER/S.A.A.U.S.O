// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyModuleTest : public VmTestBase {};

TEST_F(PyModuleTest, ModuleHasDictAndSupportsAttrReadWrite) {
  HandleScope scope;

  Handle<PyModule> module;
  if (!isolate_->factory()->NewPyModule().To(&module)) {
    FAIL() << "fail to create python module object";
  }

  Handle<PyObject> module_obj(module);

  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);
  ASSERT_FALSE(module_dict.is_null());

  // 测试__dict__获取到对象properties的能力
  Handle<PyObject> dict_value;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, module_obj, ST(dict))
                  .ToHandle(&dict_value));
  ASSERT_TRUE(IsPyDict(dict_value));
  EXPECT_TRUE(Handle<PyDict>::cast(dict_value).is_identical_to(module_dict));

  // __dict__不是存储在对象properties字典当中的实体，因此直接访问字典是查询不到的
  Handle<PyObject> mirrored_value;
  bool mirrored_found = true;
  ASSERT_TRUE(module_dict->Get(ST(dict), mirrored_value).To(&mirrored_found));
  EXPECT_FALSE(mirrored_found);
  EXPECT_TRUE(mirrored_value.is_null());

  // 同理，__class__也查询不到
  ASSERT_TRUE(module_dict->Get(ST(class), mirrored_value).To(&mirrored_found));
  EXPECT_FALSE(mirrored_found);
  EXPECT_TRUE(mirrored_value.is_null());

  Handle<PyString> x_name = PyString::NewInstance("x");
  Handle<PyObject> x_value = handle(Tagged<PyObject>(PySmi::FromInt(123)));
  ASSERT_FALSE(PyObject::SetAttr(isolate_, module_obj, x_name, x_value).IsEmpty());

  Handle<PyObject> x_got;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, module_obj, x_name).ToHandle(&x_got));
  ASSERT_TRUE(IsPySmi(x_got));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(x_got)), 123);
}

}  // namespace saauso::internal
