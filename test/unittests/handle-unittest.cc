// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

#define NR_BLOCKS(nr_handles)                                \
  ((nr_handles / HandleScopeImplementer::kHandleBlockSize) + \
   !!((nr_handles % HandleScopeImplementer::kHandleBlockSize)))

// 1. 定义一个测试夹具 (Test Fixture)
// 夹具的作用是为每个测试提供统一的环境初始化。
class HandleTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { Universe::Genesis(); }
  static void TearDownTestSuite() { Universe::Destroy(); }

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(HandleTest, EscapeFromHandleScope) {
  HandleScope scope;
  Handle<PyObject> str1 = PyString::NewInstance("Hello World");

  auto f = []() -> Handle<PyObject> {
    HandleScope scope;
    Handle<PyObject> object = PyString::NewInstance("Hello World");
    return object.EscapeFrom(&scope);
  };

  Handle<PyObject> str2 = PyString::NewInstance("I love you");
  Handle<PyObject> str3 = f();
  Handle<PyObject> str4 = PyString::NewInstance("I love you");

  EXPECT_TRUE(IsPyTrue(PyObject::Equal(str1, str3)));
  EXPECT_TRUE(IsPyTrue(PyObject::Equal(str2, str4)));
}

TEST_F(HandleTest, CreateHandlesMoreThanOneBlock) {
  HandleScope scope;

  int nr_base_block = Universe::handle_scope_implementer_->blocks().length();
  int nr_base_handles = Universe::handle_scope_implementer_->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  constexpr int kBlockNumber = 5;
  constexpr int kNumberOfHandles =
      HandleScopeImplementer::kHandleBlockSize * kBlockNumber + 100;
  for (int i = 0; i < kNumberOfHandles; ++i) {
    Handle<PySmi> object(PySmi::FromInt(1234));
  }

  int nr_expect_handles = nr_base_handles + kNumberOfHandles;
  EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(),
            nr_expect_handles);

  EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
            NR_BLOCKS(nr_expect_handles));
}

TEST_F(HandleTest, NestedHandleScopes) {
  HandleScope scope;

  int nr_base_block = Universe::handle_scope_implementer_->blocks().length();
  int nr_base_handles = Universe::handle_scope_implementer_->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  Handle<PySmi> object1(PySmi::FromInt(1));
  Handle<PySmi> object2(PySmi::FromInt(2));
  Handle<PySmi> object3(PySmi::FromInt(3));

  int outer_handles = nr_base_handles + 3;
  EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
            NR_BLOCKS(outer_handles));

  //////////////////////////////////////////////////////////////////////////////

  {
    HandleScope scope;
    constexpr int kBlockNumber = 3;
    constexpr int kNumberOfHandles =
        HandleScopeImplementer::kHandleBlockSize * kBlockNumber + 100;
    for (int i = 0; i < kNumberOfHandles; ++i) {
      Handle<PySmi> temp(PySmi::FromInt(666));
    }

    int n = outer_handles + kNumberOfHandles;
    EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(), n);
    EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
              NR_BLOCKS(n));

    {
      HandleScope scope;
      constexpr int kBlockNumber = 3;
      constexpr int kNestNumberOfHandles =
          HandleScopeImplementer::kHandleBlockSize * kBlockNumber + 100;
      for (int i = 0; i < kNestNumberOfHandles; ++i) {
        Handle<PySmi> temp(PySmi::FromInt(666));
      }

      n += kNestNumberOfHandles;
      EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(), n);
      EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
                NR_BLOCKS(n));
      n -= kNestNumberOfHandles;
    }

    EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(), n);
    EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
              NR_BLOCKS(n));
  }

  //////////////////////////////////////////////////////////////////////////////

  EXPECT_EQ(Universe::handle_scope_implementer_->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(Universe::handle_scope_implementer_->blocks().length(),
            NR_BLOCKS(outer_handles));
}

}  // namespace saauso::internal
