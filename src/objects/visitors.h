// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_VISITORS_H_
#define SAAUSO_OBJECTS_VISITORS_H_

#include "src/handles/tagged.h"

namespace saauso::internal {

class Klass;

class ObjectVisitor {
 public:
  virtual ~ObjectVisitor() {}

  // 访问一串连续的指针
  // 这里采用左闭右开区间[start, end)
  virtual void VisitPointers(Tagged<PyObject>* start,
                             Tagged<PyObject>* end) = 0;

  void VisitPointer(Tagged<PyObject>* p) { VisitPointers(p, p + 1); }

  virtual void VisitKlass(Tagged<Klass>* p) = 0;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_VISITORS_H_
