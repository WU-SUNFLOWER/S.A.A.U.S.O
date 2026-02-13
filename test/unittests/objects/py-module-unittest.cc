// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyModuleTest : public VmTestBase {};

TEST_F(PyModuleTest, ModuleHasDictAndSupportsAttrReadWrite) {
  HandleScope scope;

  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyObject> module_obj(module);

  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);
  ASSERT_FALSE(module_dict.is_null());

  // 测试__dict__获取到对象properties的能力
  Handle<PyString> dict_name = PyString::NewInstance("__dict__");
  Handle<PyObject> dict_value = PyObject::GetAttr(module_obj, dict_name, false);
  ASSERT_TRUE(IsPyDict(dict_value));
  EXPECT_TRUE(Handle<PyDict>::cast(dict_value).is_identical_to(module_dict));

  Handle<PyString> x_name = PyString::NewInstance("x");
  Handle<PyObject> x_value = handle(Tagged<PyObject>(PySmi::FromInt(123)));
  PyObject::SetAttr(module_obj, x_name, x_value);

  Handle<PyObject> x_got = PyObject::GetAttr(module_obj, x_name, false);
  ASSERT_TRUE(IsPySmi(x_got));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(x_got)), 123);
}

}  // namespace saauso::internal

