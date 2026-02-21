// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_RUNTIME_RUNTIME_CONVERSIONS_H_
#define SSAUSO_RUNTIME_RUNTIME_CONVERSIONS_H_

#include "src/handles/handles.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

class PyObject;

// 将 int/bool 等“可解释为整数”的对象解码为 int64_t。
// - 当前仅支持 Smi 与 bool；不支持时抛出 TypeError 并返回 Nothing。
Maybe<int64_t> Runtime_DecodeIntLike(Tagged<PyObject> value);

// 将 int/bool 等“可解释为整数”的对象解码为 int64_t。
// - 失败时会打印 pending exception 并直接终止进程。
int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value);

}  // namespace saauso::internal

#endif  // SSAUSO_RUNTIME_RUNTIME_CONVERSIONS_H_
