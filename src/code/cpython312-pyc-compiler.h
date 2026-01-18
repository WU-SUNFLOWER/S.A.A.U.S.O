// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_
#define SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_

#include <cstdint>
#include <string_view>
#include <vector>

namespace saauso::internal {

std::vector<uint8_t> CompilePythonSourceToPycBytes312(std::string_view source,
                                                      std::string_view filename);

void FinalizeEmbeddedPython312Runtime();

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_CPYTHON312_PYC_COMPILER_H_
