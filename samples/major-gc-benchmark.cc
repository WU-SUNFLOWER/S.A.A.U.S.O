// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "saauso.h"

using namespace saauso;

namespace {

constexpr std::string_view kPythonSource = R"(
import time

def create_cyclic_objects(n):
    """创建 n 个互相引用的 Node 对象，形成一个大的循环链表"""
    class Node:
        def __init__(self):
            self.ref = None

    # 创建 n 个 Node
    nodes = []
    i = 0
    while i < n:
        nodes.append(Node())
        i += 1

    # 构建循环：每个节点引用下一个，最后一个引用第一个
    i = 0
    while i < n:
        nodes[i].ref = nodes[(i + 1) % n]
        i += 1

    return nodes

def main(object_count):
    # 先做一次全量 GC，清理当前环境（避免残留对象影响）
    sysgc()

    # 创建大量循环引用对象，预期在 GC 结束后死亡
    create_cyclic_objects(object_count)

    # 创建大量循环引用对象，预期在 GC 结束后生还
    live_objects = create_cyclic_objects(object_count)

    start = time.perf_counter()
    sysgc()   # 执行全量 GC，回收所有不可达对象
    end = time.perf_counter()
    elapsed_ms = end - start
    print(elapsed_ms)

main(10000)
)";

}  // namespace

int main() {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);

    Local<Context> context = Context::New(isolate);
    Maybe<void> set_name_result = context->Global()->Set(
        String::New(isolate, "__name__"), String::New(isolate, "__main__"));
    if (set_name_result.IsNothing()) [[unlikely]] {
      std::cout << "set __name__ failed" << std::endl;
      return 1;
    }

    MaybeLocal<Script> maybe_script =
        Script::Compile(isolate, String::New(isolate, kPythonSource));
    maybe_script.ToLocalChecked()->Run(context);
  }

  isolate->Dispose();
  Saauso::Dispose();
  return 0;
}
