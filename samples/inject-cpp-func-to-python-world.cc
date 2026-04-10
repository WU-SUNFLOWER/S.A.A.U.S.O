// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso.h"

using namespace saauso;

namespace {

constexpr std::string_view kInjectedFuncName = "to_binary_string";

constexpr std::string_view kPythonScript = R"(
value = 2026
result = to_binary_string(value)
print(result)
)";

void ToBinaryString(FunctionCallbackInfo& info) {
  int64_t value;
  if (!info[0]->ToInteger().To(&value)) {
    // 如果 info[0] 不存在，或者它不是整型类型的，那么抛出异常
    info.ThrowRuntimeError("to_binary_string except one int argument.");
    return;
  }

  // 将整型转成二进制字符串
  std::string result;
  if (value != 0) {
    while (value > 0) {
      result = static_cast<char>(value % 2 + '0') + result;
      value /= 2;
    }
  } else {
    result = "0";
  }

  // 将 result 转换成 Python 世界对应的字符串对象。
  // 由于创建 Python 世界的字符串对象时，String::New
  // 需要明确知道在哪个 VM 实例的堆上进行创建，这里需要
  // 显式传入具体的 Isolate 单例！
  Isolate* isolate = info.GetIsolate();
  Local<String> result_str = String::New(isolate, result);

  // 将 result_str 作为函数的返回值"写回" Python 世界
  info.SetReturnValue(result_str);
}

}  // namespace

int main() {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    // 创建一个默认的全局环境
    Local<Context> context = Context::New(isolate);

    // 将 C++ 函数包装为一个 Python 世界的 Function 对象
    Local<String> injected_func_name = String::New(isolate, kInjectedFuncName);
    Local<Function> injected_func =
        Function::New(isolate, &ToBinaryString, kInjectedFuncName);

    // 将要注入的函数名作为"键"、 Function 对象作为"值"，注入进 context
    Maybe<void> set_result =
        context->Global()->Set(injected_func_name, injected_func);

    // 理论上操作不可能失败，这里的报错不可能触发！
    if (set_result.IsNothing()) [[unlikely]] {
      std::cerr << "set global failed" << std::endl;
      return 1;
    }

    // 创建并编译一段Python脚本
    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonScript));

    // 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);
  }

  isolate->Dispose();
  Saauso::Dispose();
  return 0;
}
