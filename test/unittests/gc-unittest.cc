// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "gtest/gtest.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

#define EXPECT_PY_TRUE(condition) EXPECT_TRUE(IsPyTrue(condition))

// 1. 定义一个测试夹具 (Test Fixture)
// 夹具的作用是为每个测试提供统一的环境初始化。
class GcTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { Universe::Genesis(); }
  static void TearDownTestSuite() { Universe::Destroy(); }

  void SetUp() override {}
  void TearDown() override {}
};

// 测试针对字符串的copy gc
TEST_F(GcTest, CopyGcTestForPyString) {
  HandleScope scope;

  char content[] = "Hello World";

  Handle<PyString> str1 = PyString::NewInstance(content);
  Address a1 = (*str1).ptr();
  Universe::heap_->CollectGarbage();
  Handle<PyString> str2 = PyString::NewInstance(content);
  Address a2 = (*str1).ptr();

  EXPECT_TRUE(IsPyTrue(PyObject::Equal(str1, str2)));

  EXPECT_EQ(std::strncmp(str1->buffer(), content, str1->length()), 0);

  EXPECT_NE(a1, a2);
}

// 测试针对list的copy gc
TEST_F(GcTest, CopyGcTestForPyList) {
  HandleScope scope;
  constexpr int kLength = 5;

  //////////////////////////////////////////

  Handle<PyList> list1 = PyList::NewInstance(kLength);
  Address a1 = (*list1).ptr();

  for (auto i = 0; i < kLength; ++i) {
    HandleScope scope;
    Handle<PyString> elem = PyString::NewInstance(std::to_string(i).c_str());
    PyList::Append(list1, elem);
  }

  //////////////////////////////////////////

  Universe::heap_->CollectGarbage();

  //////////////////////////////////////////

  Handle<PyList> list2 = PyList::NewInstance(kLength);

  for (auto i = 0; i < kLength; ++i) {
    HandleScope scope;
    Handle<PyString> elem = PyString::NewInstance(std::to_string(i).c_str());
    PyList::Append(list2, elem);
  }

  //////////////////////////////////////////

  EXPECT_NE(a1, (*list1).ptr());
  EXPECT_TRUE(IsPyTrue(PyObject::Equal(list1, list2)));
}

TEST_F(GcTest, CopyGcTestForForwardingPointer) {
  HandleScope scope;

  Handle<PyList> list1 = PyList::NewInstance();
  Handle<PyList> list2 = PyList::NewInstance();

  Handle<PyString> content = PyString::NewInstance("Hello World");
  PyList::Append(list1, content);
  PyList::Append(list2, content);

  Universe::heap_->CollectGarbage();

  EXPECT_PY_TRUE(PyObject::Equal(list1, list2));
  EXPECT_PY_TRUE(PyObject::Equal(list1->Get(0), list2->Get(0)));

  EXPECT_PY_TRUE(PyObject::Equal(list1->Get(0), content));
  EXPECT_PY_TRUE(PyObject::Equal(list2->Get(0), content));
}

}  // namespace saauso::internal
