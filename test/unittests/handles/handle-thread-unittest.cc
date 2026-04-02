// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <thread>

#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class HandleThreadTest : public IsolateOnlyTestBase {};

TEST_F(HandleThreadTest, ThreadHandleScopeRestoresCountsAfterThreadExit) {
  const size_t base_blocks = isolate_->handle_scope_implementer()->blocks().length();
  const int base_handles = isolate_->handle_scope_implementer()->NumberOfHandles();

  size_t observed_blocks = 0;
  int observed_handles = 0;
  Isolate* isolate = isolate_;
  std::thread t([isolate, &observed_blocks, &observed_handles] {
    isolate->Enter();
    {
      HandleScope scope(isolate);

      constexpr int kCount =
          HandleScopeImplementer::kHandleBlockSize * 2 + 123;
      for (int i = 0; i < kCount; ++i) {
        Handle<PySmi> temp(PySmi::FromInt(i), isolate);
        (void)temp;
      }

      observed_blocks = isolate->handle_scope_implementer()->blocks().length();
      observed_handles = isolate->handle_scope_implementer()->NumberOfHandles();
    }
    isolate->Exit();
  });
  t.join();

  EXPECT_GT(observed_blocks, base_blocks);
  EXPECT_GT(observed_handles, base_handles);
  EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(), base_blocks);
  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(), base_handles);
}

}  // namespace saauso::internal
