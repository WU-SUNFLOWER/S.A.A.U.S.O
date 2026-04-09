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
  Isolate* isolate = info.GetIsolate();

  Maybe<int64_t> maybe_value = info.GetIntegerArg(0);
  if (maybe_value.IsEmpty()) {
    info.ThrowRuntimeError("to_binary_string except one int argument.");
    return;
  }

  int64_t value = maybe_value.ToChecked();
  std::string result;
  if (value == 0) {
    while (value > 0) {
      result = static_cast<char>(value % 2 + '0') + result;
      value /= 2;
    }
  } else {
    result = "0";
  }

  info.SetReturnValue(String::New(isolate, result));
}

}  // namespace

int main() {
  // 初始化 S.A.A.U.S.O 库
  Saauso::Initialize();
  // 创建虚拟机实例
  Isolate* isolate = Isolate::New();

  {
    // 创建一个与 isolate 绑定的 scope，之后会自动进入 isolate 实例
    Isolate::Scope isolate_scope(isolate);

    // 创建一个 HandleScope
    HandleScope scope(isolate);

    // 创建一个默认的全局环境
    Local<Context> context = Context::New(isolate);

    Local<Object> global = context->Global().ToLocalChecked();

    Local<String> injected_func_name = String::New(isolate, kInjectedFuncName);
    Local<Function> injected_func =
        Function::New(isolate, &ToBinaryString, kInjectedFuncName)
            .ToLocalChecked();

    (void)global->Set(injected_func_name, injected_func);

    // 创建并编译一段Python脚本
    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonScript));

    // 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);

    // 此处 isolate_scope 会被析构，然后 isolate 会自动退出
  }

  // 销毁虚拟机实例
  isolate->Dispose();
  // 关闭 S.A.A.U.S.O 库
  Saauso::Dispose();
  return 0;
}
