// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/mark-word.h"

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/objects.h"
#include "src/objects/py-object.h"
#include "src/runtime/isolate.h"


namespace saauso::internal {

// static
MarkWord MarkWord::FromKlass(const Tagged<Klass> klass) {
  return MarkWord(klass.ptr());
}

// static
MarkWord MarkWord::FromForwardingAddress(const Tagged<PyObject> target_object) {
  assert(IsHeapObject(target_object) &&
         Isolate::Current()->heap()->InNewSpaceSurvivor(target_object.ptr()));
  Address value = target_object.ptr() | kForwardingTag;
  return MarkWord(value);
}

MarkWord::MarkWord(Address value) : value_(value) {
  assert(value_ != kNullAddress);
}

bool MarkWord::IsForwardingAddress() const {
  return (value_ & kForwardingTagMask) == kForwardingTag;
}

Tagged<Klass> MarkWord::ToKlass() const {
  assert(!IsForwardingAddress());
  return Tagged<Klass>(value_);
}

Tagged<PyObject> MarkWord::ToForwardingAddress() const {
  assert(IsForwardingAddress());
  Address value = value_ & (~kForwardingTagMask);
  assert(IsHeapObject(Tagged<PyObject>(value)));
  return Tagged<PyObject>(value);
}

}  // namespace saauso::internal
