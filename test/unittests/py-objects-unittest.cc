// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

// 1. 定义一个测试夹具 (Test Fixture)
// 夹具的作用是为每个测试提供统一的环境初始化。
class PyObjectTest : public testing::Test {
 protected:
  // SetUpTestSuite 会在当前文件中所有测试运行前执行一次
  // 适合做全局初始化，比如 Universe::Genesis()
  static void SetUpTestSuite() {  }
  static void TearDownTestSuite() { }

  void SetUp() override {}
  void TearDown() override { Universe::Destroy(); }
};

// 2. 测试基本数据类型的创建
// 使用 TEST_F 宏，第一个参数填上面的夹具类名 PyObjectTest
TEST_F(PyObjectTest, BasicObjectCreation) {
  HandleScope scope;  // 作用域管理
  Universe::Genesis();

  // 测试字符串
  Handle<PyObject> str_obj = PyString::NewInstance("Hello World");
  // 验证它不为空
  EXPECT_FALSE(str_obj.IsNull());
  // 验证长度：
  EXPECT_EQ(Handle<PyString>::Cast(str_obj)->length(), 11);

  // 测试 Oddballs (True/False/None)
  Handle<PyBoolean> true_obj(Universe::py_true_object_);
  Handle<PyBoolean> false_obj(Universe::py_false_object_);
  Handle<PyNone> none_obj(Universe::py_none_object_);

  EXPECT_FALSE(true_obj.IsNull());
  EXPECT_NE((*true_obj).ptr(), (*false_obj).ptr());  // 验证 True 不等于 False
}

// 3. 测试 Smi (小整数) 及其运算
TEST_F(PyObjectTest, SmiOperations) {
  HandleScope scope;
  Universe::Genesis();

  Handle<PyObject> num1(PySmi::FromInt(1234));
  Handle<PyObject> num2(PySmi::FromInt(3));

  // 测试乘法: 1234 * 3 = 3702
  Handle<PyObject> result = PyObject::Mul(num1, num2);

  // 假设你的 PySmi 有 value() 方法获取 C++ int，你应该这样断言：
  int64_t result_val = PySmi::ToInt(Handle<PySmi>::Cast(result));
  EXPECT_EQ(result_val, 3702);

  // 既然目前只能看对象存在性：
  EXPECT_FALSE(result.IsNull());
}

TEST_F(PyObjectTest, FloatOperations) {
  HandleScope scope;
  Universe::Genesis();

  Handle<PyObject> num1(PySmi::FromInt(1234));
  Handle<PyObject> num2(PyFloat::NewInstance(3.14));

  Handle<PyObject> result1 = PyObject::Mul(num1, num2);
  Handle<PyObject> result2 = PyObject::Mul(num2, num1);

  EXPECT_TRUE(IsPyFloat(result1));
  EXPECT_TRUE(IsPyFloat(result2));

  EXPECT_EQ(Handle<PyFloat>::Cast(result1)->value(),
            Handle<PyFloat>::Cast(result2)->value());

  EXPECT_EQ(Handle<PyFloat>::Cast(result1)->value(), 1234 * 3.14);
}

// 4. 测试 List 的操作
TEST_F(PyObjectTest, ListManipulation) {
  HandleScope scope;
  Universe::Genesis();

  Handle<PyList> list = PyList::NewInstance(1);
  EXPECT_EQ(list->length(), 0);  // 刚创建时长度应为0（假设语义如此）

  // 准备数据
  Handle<PyObject> item1 = PyString::NewInstance("Item 1");
  Handle<PyObject> item2(PySmi::FromInt(100));

  // Append 操作
  PyList::Append(list, item1);
  PyList::Append(list, item2);

  // 验证长度
  EXPECT_EQ(list->length(), 2);

  // 验证取值 (Get)
  Handle<PyObject> got_item1 = list->Get(0);
  EXPECT_EQ(PyObject::Equal(got_item1, item1).ptr(),
            Universe::py_true_object_.ptr());

  // 这里可以验证取出来的指针是否指向原来的对象
  // 具体的比较方式取决于你的 Handle 实现是否支持 == 重载，或者比较内部指针
  EXPECT_EQ((*got_item1).ptr(), (*item1).ptr());
}

// 5. 测试 List 的合并 (Add)
TEST_F(PyObjectTest, ListAddition) {
  HandleScope scope;
  Universe::Genesis();

  Handle<PyList> list = PyList::NewInstance(0);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list,
                 Handle<PyObject>(PySmi::FromInt(2)));  // list 现在是 [1, 2]

  // 执行 list + list
  auto combined_list = Handle<PyList>::Cast(PyList::Add(list, list));

  // 验证新列表长度应该是 4
  EXPECT_EQ(combined_list->length(), 4);

  // 验证原列表没有被修改 (如果你的逻辑是生成新列表的话)
  EXPECT_EQ(list->length(), 2);
}

}  // namespace saauso::internal
