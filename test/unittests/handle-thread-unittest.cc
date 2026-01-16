// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <thread>

#include "include/saauso.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/handles/handles.h"
#include "src/objects/py-smi.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class HandleThreadTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    saauso::Saauso::Initialize();
    isolate_ = Isolate::New();
  }
  static void TearDownTestSuite() {
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
    saauso::Saauso::Dispose();
  }

  static Isolate* isolate_;
};

Isolate* HandleThreadTest::isolate_ = nullptr;

TEST_F(HandleThreadTest, HandleScopeStateIsThreadLocalAndCleansUpAfterThread) {
  int base_blocks = 0;
  int base_handles = 0;
  base_blocks = isolate_->handle_scope_implementer()->blocks().length();
  base_handles = isolate_->handle_scope_implementer()->NumberOfHandles();

  {
    Isolate* isolate = isolate_;
    std::thread t([isolate] {
      Isolate::Locker locker(isolate);
      {
        Isolate::Scope iso_scope(isolate);
        HandleScope inner;
        constexpr int kCount =
            HandleScopeImplementer::kHandleBlockSize * 2 + 123;
        for (int i = 0; i < kCount; ++i) {
          Handle<PySmi> smi(PySmi::FromInt(i));
          (void)smi;
        }
      }
      // 离开 Scope 后，Locker 析构要求 entry_count_ == 0
    });
    t.join();
  }

  EXPECT_EQ(isolate_->handle_scope_implementer()->NumberOfHandles(),
            base_handles);
  EXPECT_EQ(isolate_->handle_scope_implementer()->blocks().length(),
            base_blocks);
}

}  // namespace saauso::internal
