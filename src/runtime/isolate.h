// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_ISOLATE_H_
#define SAAUSO_RUNTIME_ISOLATE_H_

class Heap;
class HandleScopeImplementer;

class Isolate {
 public:
  Heap* heap_;
  HandleScopeImplementer* handle_scope_implementer_;
};

#endif  // SAAUSO_RUNTIME_ISOLATE_H_
