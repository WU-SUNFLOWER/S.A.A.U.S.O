// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <thread>

#include "saauso.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {

TEST(HandleThreadTest, HandleScopeStateIsThreadLocalAndCleansUpAfterThread) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);

  size_t base_blocks = 0;
  int base_handles = 0;
  base_blocks = i_isolate->handle_scope_implementer()->blocks().length();
  base_handles = i_isolate->handle_scope_implementer()->NumberOfHandles();

  std::thread t([isolate] {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    constexpr int kCount =
        i::HandleScopeImplementer::kHandleBlockSize * 2 + 123;
    for (int i = 0; i < kCount; ++i) {
      Local<Integer> temp = Integer::New(isolate, i);
      (void)temp;
    }
  });
  t.join();

  EXPECT_EQ(i_isolate->handle_scope_implementer()->NumberOfHandles(),
            base_handles);
  EXPECT_EQ(i_isolate->handle_scope_implementer()->blocks().length(),
            base_blocks);

  isolate->Dispose();
  Saauso::Dispose();
}

}  // namespace saauso
