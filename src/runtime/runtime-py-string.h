// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_STRING_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_STRING_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyString;
class PyList;

// 将一个 Python 字符串按指定分隔符拆分为 list。
// - sep_or_null 为 null 表示按空白拆分（行为对齐 CPython 的子集语义）。
// - sep_or_null 非 null 时必须为 str；空字符串分隔符会触发 ValueError 并
// fail-fast。
// - maxsplit < 0 表示不限次数；maxsplit == 0 返回至多一个元素（保留现有行为）。
Handle<PyList> Runtime_PyStringSplit(Handle<PyString> str,
                                     Handle<PyObject> sep_or_null,
                                     int64_t maxsplit);

// 将一个可迭代对象按 Python 语义用分隔符连接为 str。
// - iterable 必须为非空且可被展开；否则会在下层 fail-fast。
// - iterable 的每个元素必须为 str，否则会打印 TypeError 并 fail-fast。
Handle<PyString> Runtime_PyStringJoin(Handle<PyString> str,
                                      Handle<PyObject> iterable);

// 按 Python 语义将任意对象转换为 str。
// - 支持 str/int/float/bool/None 的快速路径。
// - 其它对象会尝试调用 __str__；若找不到 __str__，回退为 "<object at 0x...>"。
Handle<PyString> Runtime_NewStr(Handle<PyObject> value);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_STRING_H_
