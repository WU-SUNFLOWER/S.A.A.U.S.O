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
  HandleScope scope(isolate_);
  Handle<PyObject> str1 = PyString::New(isolate_, "Hello World");

  auto f = []() -> Handle<PyObject> {
    EscapableHandleScope scope(isolate_);
    Handle<PyObject> object = PyString::New(isolate_, "Hello World");
    return scope.Escape(object);
  };

  Handle<PyObject> str2 = PyString::New(isolate_, "I love you");
  Handle<PyObject> str3 = f();
  Handle<PyObject> str4 = PyString::New(isolate_, "I love you");

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str1, str3).To(&eq));
  EXPECT_TRUE(eq);
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str2, str4).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(HandleTest, EscapableHandleScopeEscape) {
  HandleScope scope(isolate_);
  Handle<PyObject> str1 = PyString::New(isolate_, "Hello World");

  auto f = []() -> Handle<PyObject> {
    EscapableHandleScope scope(isolate_);
    Handle<PyObject> object = PyString::New(isolate_, "Hello World");
    return scope.Escape(object);
  };

  Handle<PyObject> str2 = PyString::New(isolate_, "I love you");
  Handle<PyObject> str3 = f();
  Handle<PyObject> str4 = PyString::New(isolate_, "I love you");

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str1, str3).To(&eq));
  EXPECT_TRUE(eq);
  ASSERT_TRUE(PyObject::EqualBool(isolate_, str2, str4).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(HandleTest, ExplicitIsolateHandleScopeCreatesHandles) {
  HandleScope scope(isolate_);

  auto base_handles = isolate_->handle_scope_implementer()->NumberOfHandles();

  Handle<PySmi> one = handle(PySmi::FromInt(1), isolate_);
  Handle<PySmi> two = handle(PySmi::FromInt(2), isolate_);

  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(),
            base_handles + 2);
  EXPECT_EQ(PySmi::ToInt(one), 1);
  EXPECT_EQ(PySmi::ToInt(two), 2);
}

TEST_F(HandleTest, ExplicitIsolateEscapableHandleScopeEscape) {
  HandleScope scope(isolate_);

  auto f = []() -> Handle<PySmi> {
    EscapableHandleScope scope(isolate_);
    Handle<PySmi> value = handle(PySmi::FromInt(42), isolate_);
    return scope.Escape(value);
  };

  Handle<PySmi> escaped = f();
  EXPECT_EQ(PySmi::ToInt(escaped), 42);
}

#if defined(_DEBUG) || defined(ASAN_BUILD)
TEST_F(HandleTest, ReturningUnescapedHandleShouldFailFast) {
  auto f = []() -> Handle<PyObject> {
    HandleScope scope(isolate_);
    return PyString::New(isolate_, "Hello World");
  };

  ASSERT_DEATH_IF_SUPPORTED(
      {
        HandleScope scope(isolate_);
        Handle<PyObject> bad = f();
        (void)(*bad);
      },
      "Invalid handle");
}
#endif

TEST_F(HandleTest, CreateHandlesMoreThanOneBlock) {
  HandleScope scope(isolate_);

  auto nr_base_block = isolate_->handle_scope_implementer()->blocks().length();
  auto nr_base_handles =
      isolate_->handle_scope_implementer()->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  constexpr int kBlockNumber = 5;
  constexpr int kNumberOfHandles =
      HandleScopeImplementer::kHandleBlockSize * kBlockNumber + 100;
  for (int i = 0; i < kNumberOfHandles; ++i) {
    Handle<PySmi> object(PySmi::FromInt(1234), isolate_);
  }

  int nr_expect_handles = nr_base_handles + kNumberOfHandles;
  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(),
            nr_expect_handles);

  EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(nr_expect_handles));
}

TEST_F(HandleTest, NestedHandleScopes) {
  HandleScope scope(isolate_);

  auto nr_base_block = isolate_->handle_scope_implementer()->blocks().length();
  auto nr_base_handles =
      isolate_->handle_scope_implementer()->NumberOfHandles();

  EXPECT_EQ(nr_base_block, NR_BLOCKS(nr_base_handles));

  //////////////////////////////////////////////////////////////////////////////

  Handle<PySmi> object1(PySmi::FromInt(1), isolate_);
  Handle<PySmi> object2(PySmi::FromInt(2), isolate_);
  Handle<PySmi> object3(PySmi::FromInt(3), isolate_);

  int outer_handles = nr_base_handles + 3;
  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(outer_handles));

  //////////////////////////////////////////////////////////////////////////////

  {
    HandleScope inner_scope(isolate_);
    constexpr int kInnerBlockNumber = 3;
    constexpr int kNumberOfHandles =
        HandleScopeImplementer::kHandleBlockSize * kInnerBlockNumber + 100;
    for (int i = 0; i < kNumberOfHandles; ++i) {
      Handle<PySmi> temp(PySmi::FromInt(666), isolate_);
    }

    int n = outer_handles + kNumberOfHandles;
    EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(), n);
    EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
              NR_BLOCKS(n));

    {
      HandleScope most_inner_scope(isolate_);
      constexpr int kMostInnerBlockNumber = 3;
      constexpr int kNestNumberOfHandles =
          HandleScopeImplementer::kHandleBlockSize * kMostInnerBlockNumber +
          100;
      for (int i = 0; i < kNestNumberOfHandles; ++i) {
        Handle<PySmi> temp(PySmi::FromInt(666), isolate_);
      }

      n += kNestNumberOfHandles;
      EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(), n);
      EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
                NR_BLOCKS(n));
      n -= kNestNumberOfHandles;
    }

    EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(), n);
    EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
              NR_BLOCKS(n));
  }

  //////////////////////////////////////////////////////////////////////////////

  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(),
            outer_handles);
  EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
            NR_BLOCKS(outer_handles));
}

}  // namespace saauso::internal
