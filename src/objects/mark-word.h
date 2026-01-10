// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_MARK_WORD_H_
#define SAAUSO_OBJECTS_MARK_WORD_H_

#include "include/saauso-internal.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Klass;
class PyObject;

class MarkWord {
 public:
  static MarkWord FromKlass(const Tagged<Klass> klass);
  static MarkWord FromForwardingAddress(const Tagged<PyObject> target_object);

  // 判断当前mark word是否已经是一个forwarding address
  bool IsForwardingAddress() const;

  // 从当前mark word中提取出有效的klass指针
  Tagged<Klass> ToKlass() const;
  // 从当前mark word中提取出有效的对象指针
  Tagged<PyObject> ToForwardingAddress() const;

  Address ptr() const { return value_; }

 private:
  explicit MarkWord(Address value);

  Address value_{kNullAddress};
};

}  // namespace saauso::internal

#endif
