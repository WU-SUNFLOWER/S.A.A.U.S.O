// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"

int main() {
  // 初始化 S.A.A.U.S.O 库
  saauso::Saauso::Initialize();
  // 创建虚拟机实例
  saauso::Isolate* isolate = saauso::Isolate::New();

  {
    // 创建一个与 isolate 绑定的 scope，之后会自动进入 isolate 实例
    saauso::Isolate::Scope isolate_scope(isolate);

    // 创建一个 HandleScope
    saauso::HandleScope scope(isolate);

    // 创建一个默认的全局环境
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);

    // 创建并编译一段Python脚本
    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "print('Hello World')\n"));

    // 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);

    // 此处 isolate_scope 会被析构，然后 isolate 会自动退出
  }

  // 销毁虚拟机实例
  isolate->Dispose();
  // 关闭 S.A.A.U.S.O 库
  saauso::Saauso::Dispose();
  return 0;
}
