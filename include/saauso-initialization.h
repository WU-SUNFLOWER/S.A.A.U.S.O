// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_INITIALIZATION_H_
#define INCLUDE_SAAUSO_INITIALIZATION_H_

namespace saauso {

class Saauso {
 public:
  static void Initialize();
  static void Dispose();
  static bool IsInitialized();
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_INITIALIZATION_H_
