// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso.h"

#include <atomic>
#include <cstdlib>

#if SAAUSO_ENABLE_CPYTHON_COMPILER
#include "src/code/cpython312-pyc-compiler.h"
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

namespace saauso {

namespace {
std::atomic<bool> g_initialized{false};
}  // namespace

void Saauso::Initialize() {
  bool expected = false;
  if (!g_initialized.compare_exchange_strong(expected, true)) {
    return;
  }
#if SAAUSO_ENABLE_CPYTHON_COMPILER
  internal::EmbeddedPython312Compiler::Setup();
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
}

void Saauso::Dispose() {
  bool expected = true;
  if (!g_initialized.compare_exchange_strong(expected, false)) {
    return;
  }
#if SAAUSO_ENABLE_CPYTHON_COMPILER
  internal::EmbeddedPython312Compiler::TearDown();
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
}

bool Saauso::IsInitialized() {
  return g_initialized.load();
}

}  // namespace saauso
