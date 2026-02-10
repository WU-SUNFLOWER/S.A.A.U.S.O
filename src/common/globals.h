// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_COMMON_GLOBALS_H_
#define SAAUSO_COMMON_GLOBALS_H_

namespace saauso::internal {

class AllStatic {
#ifdef _DEBUG
 public:
  AllStatic() = delete;
#endif
};

}  // namespace saauso::internal

#endif  // SAAUSO_COMMON_GLOBALS_H_
