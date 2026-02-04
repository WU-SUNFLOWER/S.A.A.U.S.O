// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef TEST_UNITTESTS_TEST_UTILS_H_
#define TEST_UNITTESTS_TEST_UTILS_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

// 单元测试中常用的无状态工具函数与断言谓词集合。
//
// 该文件只提供“辅助能力”，不引入测试夹具的生命周期管理；夹具类请见
// test-helpers.{h,cc}。
template <typename T>
class Handle;

class Isolate;
class PyCodeObject;
class PyList;
class PyObject;
class PyString;

// 获取当前 Isolate 的 None 单例（以 Handle 形式返回，便于直接参与 API 调用）。
Handle<PyObject> PyNoneObject();

// 获取当前 Isolate 的 True 单例（以 Handle 形式返回，便于直接参与 API 调用）。
Handle<PyObject> PyTrueObject();

// 获取当前 Isolate 的 False 单例（以 Handle 形式返回，便于直接参与 API 调用）。
Handle<PyObject> PyFalseObject();

// 断言 PyString 与期望文本相等。
// - s 允许为 null；为 null 时断言失败并给出原因。
// - 比较包含长度校验与逐字节比较。
::testing::AssertionResult IsPyStringEqual(Handle<PyString> s,
                                           std::string_view expected);

// 断言两个 PyObject 在 Python 语义下相等（通过 PyObject::Equal）。
// - x / y 均不允许为 null；任一为 null 则断言失败并标注哪个为空。
::testing::AssertionResult IsPyObjectEqual(Handle<PyObject> x,
                                           Handle<PyObject> y);

// 断言 condition 为 PyTrue（用于检查内建比较/谓词的返回结果）。
// - condition 不允许为 null；为 null 则断言失败。
::testing::AssertionResult IsPyTrueCondition(Handle<PyObject> condition);

// 将期望值追加到 list（用于构造解释器输出的期望序列）。
void AppendExpected(Handle<PyList> list, Handle<PyObject> value);

// 将 Python 源码编译为 CPython 3.12 的 pyc 字节流，并解析为 PyCodeObject。
// - isolate 用于解析阶段的对象分配与常量创建，不允许为 nullptr。
Handle<PyCodeObject> CompileScript312(Isolate* isolate,
                                      std::string_view source,
                                      std::string_view file_name);

// 以小端序写入 32 位整数到 out。
void PutInt32LE(std::vector<uint8_t>& out, int32_t v);

// 写入单个字节到 out。
void PutByte(std::vector<uint8_t>& out, uint8_t v);

// 写入“长度前缀 + 字节内容”的字符串，其中长度为 32 位小端整数。
void PutLongString(std::vector<uint8_t>& out, const std::string& s);

}  // namespace saauso::internal

#endif  // TEST_UNITTESTS_TEST_UTILS_H_
