// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_
#define SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_

#include <cstdint>
#include <string_view>
#include <vector>

#include "src/common/globals.h"

namespace saauso::internal {

class EmbeddedPython312Compiler : public AllStatic {
 public:
  static void Setup();

  static void TearDown();

  static std::vector<uint8_t> CompileToPycBytes(std::string_view source,
                                                std::string_view filename);
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_
