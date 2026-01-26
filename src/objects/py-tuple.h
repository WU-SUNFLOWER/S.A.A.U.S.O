// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TUPLE_H_
#define SAAUSO_OBJECTS_PY_TUPLE_H_

#include <cstddef>
#include <cstdint>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class CPython312PycFileParser;
class PycFileParser;
class PyList;

class PyTuple : public PyObject {
 public:
  static Handle<PyTuple> NewInstance(int64_t length);
  static Handle<PyTuple> NewInstance(Handle<PyList> elements);

  static Tagged<PyTuple> cast(Tagged<PyObject> object);

  static size_t ComputeObjectSize(int64_t length);

  // 仅供虚拟机内部使用，不得暴露给python语言层！
  void SetInternal(int64_t index, Handle<PyObject> value);
  void SetInternal(int64_t index, Tagged<PyObject> value);

  Handle<PyObject> Get(int64_t index) const;
  Tagged<PyObject> GetTagged(int64_t index) const;

  int64_t length() const { return length_; }

 private:
  friend class CPython312PycFileParser;
  friend class PycFileParser;
  friend class PyTupleKlass;

  Tagged<PyObject>* data() {
    return reinterpret_cast<Tagged<PyObject>*>(this + 1);
  }
  const Tagged<PyObject>* data() const {
    return reinterpret_cast<const Tagged<PyObject>*>(this + 1);
  }

  int64_t length_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_TUPLE_H_
