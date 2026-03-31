// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_COMMON_GLOBALS_H_
#define SAAUSO_COMMON_GLOBALS_H_

#include <cstdint>

namespace saauso::internal {

// - 该常量作为 Handle 毒化值，仅用于 Debug/ASAN 下的句柄失效检测，
//   承诺不参与任何运行时语义。
// - 若句柄已退化为悬空指针，也可能先由 ASAN 的 use-after-free 检测
//   拦截，而不是命中该 zap 值。
// - 毒化值最低二进制位不能是1，否则会和 Smi 冲突。
inline constexpr uint64_t kHandleZapValue = uint64_t{0x1baddead0baddeae};

class AllStatic {
#ifdef _DEBUG
 public:
  AllStatic() = delete;
#endif
};

}  // namespace saauso::internal

namespace i = saauso::internal;

#endif  // SAAUSO_COMMON_GLOBALS_H_
