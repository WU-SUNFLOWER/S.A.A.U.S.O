// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "src/execution/isolate.h"
#include "src/handles/global-handles.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class GlobalHandleTest : public VmTestBase {};

TEST_F(GlobalHandleTest, GlobalHandleShouldBeReleasedByDestructor) {
  int base =
      isolate_->handle_scope_implementer()->NumberOfGlobalHandles();

  {
    HandleScope scope(isolate_);
    Global<PyObject> g(isolate_, PyString::New(isolate_, "x"));
    EXPECT_EQ(
        isolate_->handle_scope_implementer()->NumberOfGlobalHandles(),
        base + 1);
  }

  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfGlobalHandles(), base);
}

TEST_F(GlobalHandleTest, GlobalHandleShouldSurviveAcrossGc) {
  Global<PyString> g;
  Address addr_before = kNullAddress;

  {
    HandleScope scope;
    Handle<PyString> s = PyString::New(isolate_, "global-string");
    addr_before = (*s).ptr();
    g = Global<PyString>(isolate_, s);
  }

  {
    HandleScope scope;
    isolate_->heap()->CollectGarbage();
    Address addr_after = (*g.Get(isolate_)).ptr();
    EXPECT_NE(addr_before, addr_after);
  }

  {
    HandleScope scope;
    Handle<PyString> s2 = g.Get(isolate_);
    Handle<PyString> expected = PyString::New(isolate_, "global-string");
    bool eq = false;
    ASSERT_TRUE(PyObject::EqualBool(isolate_, s2, expected).To(&eq));
    EXPECT_TRUE(eq);
  }
}

}  // namespace saauso::internal
