// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_TEMPLATES_H_
#define SAAUSO_OBJECTS_TEMPLATES_H_

#include "src/handles/global-handles.h"
#include "src/objects/native-function-helpers.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class FunctionTemplateInfo {
 public:
  explicit FunctionTemplateInfo(NativeFuncPointer function,
                                Handle<PyString> name)
      : function_(function), name_(name) {}

  NativeFuncPointer function() const { return function_; }
  Handle<PyString> name() const { return name_.Get(); }

 private:
  NativeFuncPointer function_;
  Global<PyString> name_;
};

}  // namespace saauso::internal

#endif  // INCLUDE_SAAUSO_NATIVE_FUNCTION_H_
