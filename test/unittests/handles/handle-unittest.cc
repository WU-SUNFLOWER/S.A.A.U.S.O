// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

#define NR_BLOCKS(nr_handles)                                \
  ((nr_handles / HandleScopeImplementer::kHandleBlockSize) + \
   !!((nr_handles % HandleScopeImplementer::kHandleBlockSize)))

class HandleTest : public VmTestBase {};

TEST_F(HandleTest, EscapeFromHandleScope) {
  HandleScope scope;
  Handle<PyObject> str1 = PyString::NewInstance("Hello World");

  auto f = []() -> Handle<PyObject> {
    EscapableHandleScope scope;
    Handle<PyObject> object = PyString::NewInstance("Hello World");
    return scope.Escape(object);
  };

  Handle<PyObject> str2 = PyString::NewInstance("I love you");
  Handle<PyObject> str3 = f();
  Handle<PyObject> str4 = PyString::NewInstance("I love you");

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str1, str3).To(&eq));
  EXPECT_TRUE(eq);
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str2, str4).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(HandleTest, EscapableHandleScopeEscape) {
  HandleScope scope;
  Handle<PyObject> str1 = PyString::NewInstance("Hello World");

  auto f = []() -> Handle<PyObject> {
    EscapableHandleScope scope;
    Handle<PyObject> object = PyString::NewInstance("Hello World");
    return scope.Escape(object);
  };

  Handle<PyObject> str2 = PyString::NewInstance("I love you");
  Handle<PyObject> str3 = f();
  Handle<PyObject> str4 = PyString::NewInstance("I love you");

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str1, str3).To(&eq));
  EXPECT_TRUE(eq);
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str2, str4).To(&eq));
  EXPECT_TRUE(eq);
}

#if defined(_DEBUG) || defined(ASAN_BUILD)
TEST_F(HandleTest, ReturningUnescapedHandleShouldFailFast) {
  auto f = []() -> Handle<PyObject> {
    HandleScope scope;
    return PyString::NewInstance("Hello World");
  };

  ASSERT_DEATH_IF_SUPPORTED(
      {
        HandleScope scope;
        Handle<PyObject> bad = f();
        (void)(*bad);
      },
      "Invalid handle");
}
#endif

TEST_F(HandleTest, CreateHandlesMoreThanOneBlock) {
  HandleScope scope;

  auto nr_base_block =
      Isolate::Current()->handle_scope_implementer()->blocks().length();
  auto nr_base_handles =
      Isolate::Current()->handle_scope_implementer()->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  constexpr int kBlockNumber = 5;
  constexpr int kNumberOfHandles =
      HandleScopeImplementer::kHandleBlockSize * kBlockNumber + 100;
  for (int i = 0; i < kNumberOfHandles; ++i) {
    Handle<PySmi> object(PySmi::FromInt(1234));
  }

  int nr_expect_handles = nr_base_handles + kNumberOfHandles;
  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->NumberOfHandles(),
            nr_expect_handles);

  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(nr_expect_handles));
}

TEST_F(HandleTest, NestedHandleScopes) {
  HandleScope scope;

  auto nr_base_block =
      Isolate::Current()->handle_scope_implementer()->blocks().length();
  auto nr_base_handles =
      Isolate::Current()->handle_scope_implementer()->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  Handle<PySmi> object1(PySmi::FromInt(1));
  Handle<PySmi> object2(PySmi::FromInt(2));
  Handle<PySmi> object3(PySmi::FromInt(3));

  int outer_handles = nr_base_handles + 3;
  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(outer_handles));

  //////////////////////////////////////////////////////////////////////////////

  {
    HandleScope inner_scope;
    constexpr int kInnerBlockNumber = 3;
    constexpr int kNumberOfHandles =
        HandleScopeImplementer::kHandleBlockSize * kInnerBlockNumber + 100;
    for (int i = 0; i < kNumberOfHandles; ++i) {
      Handle<PySmi> temp(PySmi::FromInt(666));
    }

    int n = outer_handles + kNumberOfHandles;
    EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->NumberOfHandles(),
              n);
    EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->blocks().length(),
              NR_BLOCKS(n));

    {
      HandleScope most_inner_scope;
      constexpr int kMostInnerBlockNumber = 3;
      constexpr int kNestNumberOfHandles =
          HandleScopeImplementer::kHandleBlockSize * kMostInnerBlockNumber +
          100;
      for (int i = 0; i < kNestNumberOfHandles; ++i) {
        Handle<PySmi> temp(PySmi::FromInt(666));
      }

      n += kNestNumberOfHandles;
      EXPECT_EQ(
          Isolate::Current()->handle_scope_implementer()->NumberOfHandles(), n);
      EXPECT_EQ(
          Isolate::Current()->handle_scope_implementer()->blocks().length(),
          NR_BLOCKS(n));
      n -= kNestNumberOfHandles;
    }

    EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->NumberOfHandles(),
              n);
    EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->blocks().length(),
              NR_BLOCKS(n));
  }

  //////////////////////////////////////////////////////////////////////////////

  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(Isolate::Current()->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(outer_handles));
}

}  // namespace saauso::internal
