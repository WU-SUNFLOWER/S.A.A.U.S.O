// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/build/build_config.h"
#include "src/build/buildflag.h"

#if BUILDFLAG(IS_LINUX)
extern "C" const char* __lsan_default_suppressions() {
  return "leak:PyUnicode_New\n";
}
#endif

