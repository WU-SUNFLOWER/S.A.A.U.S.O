// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "saauso.h"

int main() {
  saauso::Saauso::Initialize();
  // 创建虚拟机实例
  saauso::Isolate* isolate = saauso::Isolate::New();

  {
    saauso::Isolate::Scope isolate_scope(isolate);
    saauso::HandleScope scope(isolate);

    // 创建一个默认的全局环境
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    saauso::Context::Scope context_scope(context);

    // 创建一个虚拟机异常捕获器
    saauso::TryCatch try_catch(isolate);

    // 创建并编译一段Python脚本
    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "print('Hello World')\n"));
    // 如果编译发生错误，退出
    if (maybe_script.IsEmpty() || try_catch.HasCaught()) {
      return 1;
    }

    // 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);
  }

  // 销毁虚拟机实例
  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
