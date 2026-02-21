// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_
#define SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_

#include "src/builtins/builtins-utils.h"

namespace saauso::internal {

// builtins-io
BUILTIN(Print);

// builtins-refection
BUILTIN(Len);
BUILTIN(IsInstance);
BUILTIN(BuildTypeObject);

// builtins-exec
BUILTIN(Exec);

// builtins-debug
BUILTIN(Sysgc);

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_
