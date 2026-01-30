// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_INTERPRETER_FRAME_OBJECT_BUILDER_H_
#define SAAUSO_INTERPRETER_FRAME_OBJECT_BUILDER_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class FrameObject;
class PyCodeObject;
class PyDict;
class PyFunction;
class PyObject;
class PyTuple;

// FrameObjectBuilder 负责“创建 FrameObject + 调用参数绑定/打包”的全部逻辑。
//
// 设计要点：
// 1) FrameObject 只作为栈帧数据容器（字段存储/operand stack/bytecode
// 读取/Iterate）。 2) Builder 保留两条创建路径：
//    - SlowPath：输入为 pos_args + kw_args(dict)，需要遍历 dict 做形参匹配。
//    - FastPath：输入为 actual_args(tuple) + kwarg_keys(tuple)，仅用 tuple
//    索引完成 kw 处理，
//      避免 dict 迭代与额外中间结构，保证高性能。
// 3) 当前错误处理策略与旧实现一致：stderr +
// exit(1)，后续异常体系完善时再统一替换。
class FrameObjectBuilder {
 public:
  FrameObjectBuilder() = delete;

  // 仅用于创建根栈帧（Interpreter::Run 的入口帧）。
  static FrameObject* BuildRootFrame(Handle<PyCodeObject> code_object);

  // 创建一般的 python 栈帧（慢速路径）。
  // - 遍历 actual_kw_args：若 key 命中形参则写入 localsplus，否则注入 **kwargs
  // 或报错。
  // - 支持默认参数回填、*args/**kwargs 的打包与注入。
  static FrameObject* BuildSlowPath(Handle<PyFunction> func,
                                    Handle<PyObject> host,
                                    Handle<PyTuple> actual_pos_args,
                                    Handle<PyDict> actual_kw_args);

  // 创建一般的 python 栈帧（快速路径）。
  // - 约定：所有 kw 值位于 actual_args 尾部，与 kwarg_keys 一一对应。
  // - 遍历 kwarg_keys（从尾部取 key/value），尽量用 tuple 索引完成处理。
  // - 同样支持默认参数回填、*args/**kwargs 的打包与注入。
  static FrameObject* BuildFastPath(Handle<PyFunction> func,
                                    Handle<PyObject> host,
                                    Handle<PyTuple> actual_args,
                                    Handle<PyTuple> kwarg_keys);
};

}  // namespace saauso::internal

#endif  // SAAUSO_INTERPRETER_FRAME_OBJECT_BUILDER_H_
