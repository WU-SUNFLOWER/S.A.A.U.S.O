// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "saauso.h"

namespace {

constexpr std::string_view kPythonSource = R"(
import time

def generate_data(n):
    i = n
    data = []
    while i >= 0:
        data.append(i)
        i -= 1
    return data

def bubble_sort(arr):
    n = len(arr)
    i = 0
    while i < n - 1:
        swapped = False
        j = 0
        while j < n - i - 1:
            if arr[j] > arr[j + 1]:
                arr[j], arr[j + 1] = arr[j + 1], arr[j]
                swapped = True
            j += 1
        if not swapped:
            break
        i += 1
    return arr

def main():
    data = generate_data(10000)

    start_time = time.perf_counter()
    sorted_data = bubble_sort(data)
    end_time = time.perf_counter()

    elapsed = end_time - start_time
    print(elapsed)

main()
)";

}

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
        isolate, saauso::String::New(isolate, kPythonSource));

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
