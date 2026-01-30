// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "src/handles/global-handles.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class GlobalHandleTest : public VmTestBase {};

TEST_F(GlobalHandleTest, GlobalHandleShouldBeReleasedByDestructor) {
  int base =
      Isolate::Current()->handle_scope_implementer()->NumberOfGlobalHandles();

  {
    HandleScope scope;
    Global<PyObject> g(PyString::NewInstance("x"));
    EXPECT_EQ(
        Isolate::Current()->handle_scope_implementer()->NumberOfGlobalHandles(),
        base + 1);
  }

  EXPECT_EQ(
      Isolate::Current()->handle_scope_implementer()->NumberOfGlobalHandles(),
      base);
}

TEST_F(GlobalHandleTest, GlobalHandleShouldSurviveAcrossGc) {
  Global<PyString> g;
  Address addr_before = kNullAddress;

  {
    HandleScope scope;
    Handle<PyString> s = PyString::NewInstance("global-string");
    addr_before = (*s).ptr();
    g = Global<PyString>(s);
  }

  {
    HandleScope scope;
    Isolate::Current()->heap()->CollectGarbage();
    Address addr_after = (*g.Get()).ptr();
    EXPECT_NE(addr_before, addr_after);
  }

  {
    HandleScope scope;
    Handle<PyString> s2 = g.Get();
    Handle<PyString> expected = PyString::NewInstance("global-string");
    EXPECT_TRUE(PyObject::EqualBool(s2, expected));
  }
}

}  // namespace saauso::internal
