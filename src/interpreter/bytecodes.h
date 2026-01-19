// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

namespace saauso::internal {

#define BYTECODE_LIST(V) \
  V(PopTop, 1)           \
  V(PushNull, 2)         \
  V(LoadConst, 100)      \
  V(LoadName, 101)       \
  V(ReturnConst, 121)    \
  V(Resume, 151)         \
  V(Call, 171)

#define DECL_BYTECODES(name, value) inline constexpr uint8_t name = value;
BYTECODE_LIST(DECL_BYTECODES)
#undef DECL_BYTECODES

}  // namespace saauso::internal
