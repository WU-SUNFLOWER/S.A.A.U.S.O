// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso.h"

using namespace saauso;

namespace {

constexpr std::string_view kPythonFuncName = "to_binary_string";

constexpr std::string_view kPythonScript = R"(
def to_binary_string(value):
  result = ""
  
  if value == 0:
    return "0"
  
  while value > 0:
    result = str(value % 2) + result
    value //= 2
  return result
)";

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

    // 创建并编译一段Python脚本
    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonScript));

    // 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);

    Local<Object> global = context->Global().ToLocalChecked();
    MaybeLocal<Value> maybe_to_binary_string =
        global->Get(String::New(isolate, kPythonFuncName));
    Local<Function> to_binary_string =
        Local<Function>::Cast(maybe_to_binary_string.ToLocalChecked());

    Local<Value> argv[] = {Integer::New(isolate, 2026)};
    MaybeLocal<Value> result =
        to_binary_string->Call(context, Local<Value>(), 1, argv);

    Maybe<std::string> result_in_std = result.ToLocalChecked()->ToString();
    std::cout << result_in_std.ToChecked() << std::endl;

    // 此处 isolate_scope 会被析构，然后 isolate 会自动退出
  }

  // 销毁虚拟机实例
  isolate->Dispose();
  // 关闭 S.A.A.U.S.O 库
  Saauso::Dispose();
  return 0;
}
