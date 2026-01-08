// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_STRING_H_
#define SAAUSO_OBJECTS_STRING_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyString : public PyObject {
 public:
  static Handle<PyString> NewInstance(int64_t length);
  static Handle<PyString> NewInstance(const char* source);
  static Handle<PyString> NewInstance(const char* source, int64_t length);

  static Tagged<PyString> Cast(Tagged<PyObject> object);

  void Set(int64_t index, char value);
  char Get(int64_t index) const;

  uint64_t GetHash();
  bool HasHashCache() const;

  bool IsLessThan(Tagged<PyString> other);
  bool IsEqualTo(Tagged<PyString> other);
  bool IsGreaterThan(Tagged<PyString> other);

  int64_t length() const { return length_; }
  const char* buffer() const { return buffer_; }

  // 特别提醒：
  // 调用任何可能会触发 GC（或者参数计算可能触发 GC）的虚函数时，
  // 必须通过 Handle 进行调用（即 -> 的左边必须是一个 Handle 对象）！！！

  static Handle<PyString> Slice(Handle<PyString> self,
                                int64_t from,
                                int64_t to);
  static Handle<PyString> Append(Handle<PyString> self, Handle<PyString> other);

 private:
  // TODO: 实现Visitor入口函数，将PyString和缓冲区拷贝到survivor区
  char* buffer_{nullptr};
  int64_t length_{0};
  uint64_t hash_{0};
};

}  // namespace saauso::internal

#endif
