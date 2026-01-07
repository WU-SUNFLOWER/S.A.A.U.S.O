// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_UNIVERSE_H_
#define SAAUSO_RUNTIME_UNIVERSE_H_

namespace saauso::internal {

class Heap;
class Klass;
class PyObject;
class PyBoolean;
class PyNone;

class Universe {
 public:
  static Heap* heap_;

  static PyNone* py_none_object_;
  static PyBoolean* py_true_object_;
  static PyBoolean* py_false_object_;

  static void Genesis();
  static void Destroy();

  static PyBoolean* ToPyBoolean(bool condition) {
    return condition ? py_true_object_ : py_false_object_;
  }

 private:
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_UNIVERSE_H_
