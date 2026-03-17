// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_TUPLE_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_TUPLE_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyString;
class PyTuple;
class Isolate;

// 生成 tuple 的 repr 字符串。
// - tuple 不允许为 null，由调用方保证。
// - 返回值为新建的 str；若元素 repr 过程中抛异常则返回 empty。
MaybeHandle<PyString> Runtime_NewTupleRepr(Isolate* isolate,
                                           Handle<PyTuple> tuple);

// 复制 tuple 的半开区间 [begin, end) 并返回新 tuple。
// - tuple 允许为 null；为 null 时直接返回 null Handle。
// - begin/end 会被裁剪到合法范围，且保证 end >= begin。
// - 返回值始终为拷贝结果（可为空 tuple）。
Handle<PyTuple> Runtime_NewTupleSlice(Handle<PyTuple> tuple,
                                      int64_t begin,
                                      int64_t end);

// 返回 tuple 从 begin 开始的尾部切片；常用于实参去首元素场景。
// - tuple 允许为 null；为 null 时返回 null Handle。
// - 当 begin >= tuple 长度时返回 null Handle。
// - 其余情况等价于 Runtime_NewTupleSlice(tuple, begin, tuple->length())。
Handle<PyTuple> Runtime_NewTupleTailOrNull(Handle<PyTuple> tuple,
                                           int64_t begin);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_TUPLE_H_
