// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/objects/visitors.h"

namespace saauso::internal {

void Klass::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&name_);
}

}  // namespace saauso::internal
