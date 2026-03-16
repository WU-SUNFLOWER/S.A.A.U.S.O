// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef TEST_UNITTESTS_TEST_HELPERS_H_
#define TEST_UNITTESTS_TEST_HELPERS_H_

#include <string>
#include <string_view>

#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

// 单元测试夹具与公共测试基类。
//
// 该文件聚焦于测试生命周期与运行环境搭建（Initialize/Dispose、Isolate
// 创建等）； 与之配套的无状态断言与小工具函数见 test-utils.{h,cc}。
template <typename T>
class Global;

template <typename T>
class Handle;

template <typename T>
class MaybeHandle;

class Isolate;
class PyDict;
class PyList;
class PyObject;
class PyTuple;

// 需要完整虚拟机环境（包含 Isolate Enter/Exit）的单元测试基类。
class VmTestBase : public ::testing::Test {
 protected:
  // 在整个测试套件启动时初始化运行时并创建/进入 Isolate。
  static void SetUpTestSuite();

  // 在整个测试套件结束时退出/销毁 Isolate 并释放运行时。
  static void TearDownTestSuite();

  static Isolate* isolate_;
};

// 仅创建/销毁 Isolate、但不 Enter 的单元测试基类（用于隔离/并发等场景）。
class IsolateOnlyTestBase : public ::testing::Test {
 protected:
  // 在整个测试套件启动时初始化运行时并创建 Isolate（不 Enter）。
  static void SetUpTestSuite();

  // 在整个测试套件结束时销毁 Isolate 并释放运行时。
  static void TearDownTestSuite();

  static Isolate* isolate_;
};

// 基础解释器端到端测试夹具：将 builtins.print 注入为 Builtin_PrintV，
// 并把每次 print 的实参收集到列表中供断言使用。
class BasicInterpreterTest : public VmTestBase {
 protected:
  // 初始化解释器环境，并注入 builtins.print。
  static void SetUpTestSuite();

  // 清理收集结果并释放运行时资源。
  static void TearDownTestSuite();

  // 每个测试用例开始前重置输出收集容器。
  void SetUp() override;

  // 编译并运行指定 Python 源码（CPython 3.12 字节码）。
  static void RunScript(std::string_view source, std::string_view file_name);

  static std::string ExpectedAndTakePendingExceptionMessage();

  static void RunScriptExpectExceptionContains(std::string_view source,
                                               std::string_view expected_substr,
                                               std::string_view file_name);

  // 断言当前用例收集到的 print 实参序列与 expected 相等。
  static void ExpectPrintResult(Handle<PyList> expected);

  // 获取当前解释器测试使用的 Isolate。
  static Isolate* isolate();

 private:
  // 测试用 native print：把 args 中的每个实参追加到 printv_result_。
  static MaybeHandle<PyObject> Builtin_PrintV(Isolate* isolate,
                                              Handle<PyObject> host,
                                              Handle<PyTuple> args,
                                              Handle<PyDict> kwargs);

  static Global<PyList> printv_result_;
};

// 解释器端到端测试默认使用的虚拟文件名（用于编译与错误定位信息）。
inline constexpr std::string_view kInterpreterTestFileName = "saauso_test.py";

}  // namespace saauso::internal

#endif  // TEST_UNITTESTS_TEST_HELPERS_H_
