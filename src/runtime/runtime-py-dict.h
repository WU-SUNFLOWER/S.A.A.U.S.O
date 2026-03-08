// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;
class PyDict;

MaybeHandle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                      Handle<PyObject> kwargs);

// - 成功：返回有效 Maybe，同时 result 被正确初始化
// - 失败：返回空
Maybe<void> Runtime_InitDictFromArgsKwargs(Handle<PyDict> result,
                                           Handle<PyObject> args,
                                           Handle<PyObject> kwargs);

// dict 相关 API 约定（请务必阅读）
//
// 1) runtime 语义入口（推荐）
// - 当调用方需要 Python 语言层语义时，优先使用 runtime 入口：
//   - d[key] miss -> KeyError
//   - del d[key] miss -> KeyError
//   - __hash__/__eq__ 触发的异常必须原样传播（pending exception）
// - 该类语义应集中治理（klass 虚方法、builtins、runtime helper
// 等应复用同一入口）。
//
// 2) PyDict 原语（允许，但必须使用 fallible API）
// - 当调用方实现的是“解释器更高层/更特化语义”，且不等价于 d[key] 时，
//   允许直接使用 PyDict 原语（例如 LOAD_NAME/LOAD_GLOBAL 的
//   locals->globals->builtins 链式查找：miss 继续，最终 NameError）。
// - 这类场景必须使用 PyDict 的 fallible
// API（Get/GetTagged/Put/Remove/
//   ContainsKey），并在失败（Maybe 为空）时立即向上传播 pending exception；
//   禁止把“异常”误判为“未命中”。
MaybeHandle<PyObject> Runtime_DictGetItem(Handle<PyDict> dict,
                                          Handle<PyObject> key);
MaybeHandle<PyObject> Runtime_DictSetItem(Handle<PyDict> dict,
                                          Handle<PyObject> key,
                                          Handle<PyObject> value);
MaybeHandle<PyObject> Runtime_DictDelItem(Handle<PyDict> dict,
                                          Handle<PyObject> key);

MaybeHandle<PyObject> Runtime_DictGet(Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null);
MaybeHandle<PyObject> Runtime_DictSetDefault(Handle<PyDict> dict,
                                             Handle<PyObject> key,
                                             Handle<PyObject> default_or_null);
MaybeHandle<PyObject> Runtime_DictPop(Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null,
                                      bool has_default);

MaybeHandle<PyObject> Runtime_MergeDict(Isolate* isolate,
                                        Handle<PyDict> dst_dict,
                                        Handle<PyDict> source_dict,
                                        bool allow_overwriting);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_DICT_H_
