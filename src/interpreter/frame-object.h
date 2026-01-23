// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_INTERPRETER_FRAME_OBJECT_H_
#define SAAUSO_INTERPRETER_FRAME_OBJECT_H_

#include "src/handles/handles.h"
#include "src/objects/objects.h"

namespace saauso::internal {

class PyObject;
class PyCodeObject;
class PyTuple;
class PyDict;
class PyList;
class PyFunction;
class FixedArray;

class FrameObject : Object {
 public:
  explicit FrameObject(Handle<PyCodeObject> code);
  explicit FrameObject(Handle<PyFunction> func,
                       Handle<PyObject> host,
                       Handle<PyTuple> args);
  ~FrameObject();

  void PushToStack(Tagged<PyObject> object);
  void PushToStack(Handle<PyObject> object);
  Handle<PyObject> PopFromStack();

  bool HasMoreCodes();
  uint8_t GetOpCode();
  int GetOpArg();

  bool IsFirstFrame() { return caller_ == nullptr; };

  void set_pc(int pc) { pc_ = pc; }
  int pc() const { return pc_; }

  FrameObject* caller() const { return caller_; }
  void set_caller(FrameObject* caller) { caller_ = caller; }

  Handle<FixedArray> stack() const;
  Handle<PyTuple> consts() const;
  Handle<PyTuple> names() const;
  Handle<PyDict> locals() const;
  Handle<FixedArray> fast_locals() const;
  Handle<PyDict> globals() const;
  Handle<PyCodeObject> code_object() const;

 private:
  // FixedArray* stack_;
  Tagged<PyObject> stack_{kNullAddress};
  int stack_top_{0};  // stack_top_指向栈中下一个可以放置元素位置的下标

  // PyTuple* consts;
  Tagged<PyObject> consts_{kNullAddress};
  // PyTuple* names;
  Tagged<PyObject> names_{kNullAddress};

  // PyDict* locals; 支持通过符号名称泄露给外部的变量表
  Tagged<PyObject> locals_{kNullAddress};
  // FixedArray* fast_locals
  Tagged<PyObject> fast_locals_{kNullAddress};
  // PyDict* globals;
  Tagged<PyObject> globals_{kNullAddress};

  // PyCodeObject* code_object;
  Tagged<PyObject> code_object_{kNullAddress};
  int64_t pc_{0};

  FrameObject* caller_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_INTERPRETER_FRAME_OBJECT_H_
