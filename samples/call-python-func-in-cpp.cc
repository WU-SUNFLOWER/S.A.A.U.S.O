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
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    // 创建一个默认的全局环境
    Local<Context> context = Context::New(isolate);

    // 创建并编译一段Python脚本
    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonScript));

    maybe_script.ToLocalChecked()->Run(context);

    /////////////////////////////////////////////////////////////////////////

    // 要获取的全局函数名称。
    Local<String> func_name = String::New(isolate, kPythonFuncName);

    // 通过 Context 实例，拿到 Python 函数在 C++ 世界对应的 Function 对象
    auto func = Local<Function>::Cast(
        context->Global()->Get(func_name).ToLocalChecked());

    // 调用 Function 对象，并保存返回结果
    Local<Value> argv[] = {Integer::New(isolate, 2026)};
    MaybeLocal<Value> result = func->Call(context, Local<Value>(), 1, argv);

    // 将返回得到 Value 转成 std::string，并打印
    Maybe<std::string> result_in_std = result.ToLocalChecked()->ToString();
    std::cout << result_in_std.ToChecked() << std::endl;
  }

  isolate->Dispose();
  Saauso::Dispose();
  return 0;
}
