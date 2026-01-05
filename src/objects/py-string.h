// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_STRING_H_
#define SAAUSO_OBJECTS_STRING_H_

#include <cstdint>

#include "handles/handles.h"
#include "objects/py-object.h"

namespace saauso::internal {

class PyString : public PyObject {
 public:
  static Handle<PyString> NewInstance(int length);
  static Handle<PyString> NewInstance(const char* source, int length);

  static PyString* Cast(PyObject* object);

  void Set(int index, char value);
  char Get(int index) const;

  uint64_t GetHash();
  bool HasHashCache() const;

  bool IsLessThan(PyString* other);
  bool IsEqualTo(PyString* other);
  bool IsLargerThan(PyString* other);

  int length() const { return length_; }
  const char* buffer() const { return buffer_; }

  // 触发GC的方法均单独实现成static函数
  static Handle<PyString> Slice(Handle<PyString> string, int from, int to);
  static Handle<PyString> Append(Handle<PyString> string,
                                 Handle<PyString> other);

 private:
  // TODO: 实现Visitor入口函数，将PyString和缓冲区拷贝到survivor区
  char* buffer_{nullptr};
  int length_{0};
  uint64_t hash_{0};
};

}  // namespace saauso::internal

#endif
