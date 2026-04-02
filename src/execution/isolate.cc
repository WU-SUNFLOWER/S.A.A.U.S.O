// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"

#include <thread>
#include <vector>

#include "src/execution/thread-state-infras.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/interpreter/builtin-bootstrapper.h"
#include "src/interpreter/interpreter.h"
#include "src/modules/module-manager.h"
#include "src/objects/cell-klass.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception-klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"
#include "src/utils/random-number-generator.h"

namespace saauso::internal {

namespace {
thread_local std::vector<Isolate*> g_entered_isolates;
}  // namespace

thread_local Isolate* Isolate::current_ = nullptr;

Isolate* Isolate::New() {
  auto* isolate = new Isolate();
  isolate->Init();
  return isolate;
}

void Isolate::Dispose(Isolate* isolate) {
  if (isolate == nullptr) {
    return;
  }

  // 必须确保所有线程都已退出该 Isolate，才能销毁它。
  assert(!isolate->IsInUse());

  isolate->TearDown();
  delete isolate;
}

// static
Isolate* Isolate::Current() {
  return current_;
}

void Isolate::Enter() {
  ThreadId tid = ThreadId::Current();
  // 合法进入该 Isolate 的情况：
  // - 该 Isolate 没有被任何线程进入过
  // - 该 Isolate 重复被当前线程进入
  assert(owner_thread_ == ThreadId{} || owner_thread_ == tid);

  // 标记该 Isolate 被当前线程进入
  owner_thread_ = tid;

  // 将之前的 current Isolate 压栈。
  // 这个设计是为了支持嵌套进入不同的 Isolate
  // 的场景（虽然实际上这种写法很少见）：
  // {
  //   Isolate::Scope isolate_scope1(isolate1);
  //   {
  //     Isolate::Scope isolate_scope2(isolate2);
  //     // ...
  //   }
  // }
  g_entered_isolates.push_back(current_);
  // 更新当前线程的 TLS 变量，将当前线程进入的这个 Isolate 实例作为 current
  current_ = this;

  // 该 Isolate 实例的进入计数器自增 1
  ++entry_count_;
}

void Isolate::Exit() {
  ThreadId tid = ThreadId::Current();

  // 退出该 Isolate 的线程，和进入该 Isolate 的必须是同一个
  assert(owner_thread_ == tid);
  // 准备退出的该 Isolate，必须是 Current Isolate
  assert(current_ == this);
  // 该 Isolate 实例的进入计数器不可能已经清零
  assert(entry_count_ > 0);

  // 该 Isolate 实例的进入计数器自减 1
  --entry_count_;

  // 恢复之前的 current_ Isolate
  current_ = g_entered_isolates.back();
  g_entered_isolates.pop_back();

  // 如果当前线程已经彻底退出该 Isolate，那么清空标记
  if (entry_count_ == 0) {
    owner_thread_ = ThreadId{};
  }
}

void Isolate::Init() {
  // 初始化线程管理器
  thread_manager_ = new ThreadManager(this);

  // 初始化堆（Heap）
  heap_ = new Heap(this);
  heap_->Setup();

  factory_ = new Factory(this);

  // 初始化句柄作用域实现
  handle_scope_implementer_ = new HandleScopeImplementer(this);

  // 进入当前 Isolate 作用域，以便后续的初始化操作（如分配对象）
  // 能找到正确的 Isolate
  // TODO: VM内部所有链路都消除显式的 Isolate::Current() 之后，移除这里的
  // Enter / Exit 操作。
  Enter();
  do {
    HandleScope scope(this);

    // 初始化元数据区域（Klasses, Singletons 等）
    if (InitMetaArea().IsNothing()) {
      break;
    }

    // 初始化解释器
    interpreter_ = new Interpreter(this);

    // 初始化模块管理器（sys.modules/sys.path 等）。
    module_manager_ = new ModuleManager(this);

    // 初始化每个 Isolate 独立持有的随机数状态。
    random_number_generator_ = new RandomNumberGenerator();

    // 初始化内建对象字典
    Handle<PyDict> builtins;
    if (!BuiltinBootstrapper(this).CreateBuiltins().ToHandle(&builtins)) {
      break;
    }
    builtins_ = *builtins;

    // 标记为已经初始化完毕
    initialized_ = true;
  } while (0);
  Exit();
}

Maybe<void> Isolate::InitMetaArea() {
  // 1. 预初始化所有 Klass（设置 vtable，调用 PreInitialize）
#define PREINIT_PY_KLASS(_, Klass, __)     \
  do {                                     \
    auto klass = Klass::GetInstance(this); \
    klass->PreInitialize(this);            \
  } while (false);
  ISOLATE_KLASS_LIST(PREINIT_PY_KLASS)
#undef PREINIT_PY_KLASS

  // 2. 初始化字符串表
  string_table_ = new StringTable(this);

  // 3. 创建全局单例对象（None, True, False）
  // 这些对象必须在其他逻辑使用它们之前创建好
  py_none_object_ = factory()->NewPyNone();
  py_true_object_ = factory()->NewPyBoolean(true);
  py_false_object_ = factory()->NewPyBoolean(false);

  // 4. 正式初始化所有 Klass（调用 Initialize）
#define INIT_PY_KLASS(ignore1, klass, ignore2)                  \
  if (klass::GetInstance(this)->Initialize(this).IsNothing()) { \
    return kNullMaybe;                                          \
  }

  ISOLATE_KLASS_LIST(INIT_PY_KLASS)
#undef INIT_PY_KLASS

  return JustVoid();
}  // namespace saauso::internal

bool Isolate::HasPendingException() const {
  return exception_state_.HasPendingException();
}

Tagged<PyDict> Isolate::raw_builtins() const {
  return Tagged<PyDict>::cast(builtins_);
}

Handle<PyDict> Isolate::builtins() {
  assert(!raw_builtins().is_null());
  return handle(raw_builtins(), this);
}

void Isolate::Iterate(ObjectVisitor* v) {
  // 遍历内建对象字典
  v->VisitPointer(&builtins_);
}

void Isolate::TearDown() {
  // TODO: VM内部所有链路都消除显式的 Isolate::Current() 之后，移除这里的
  // Enter / Exit 操作。
  Enter();
  do {
    // 反向操作：先销毁 Klass
#define FINALIZE_PY_KLASS(_, Klass, __) \
  Klass::GetInstance(this)->Finalize(this);
    ISOLATE_KLASS_LIST(FINALIZE_PY_KLASS)
#undef FINALIZE_PY_KLASS

    // 销毁字符串表
    delete string_table_;
    string_table_ = nullptr;

    // 销毁解释器
    delete interpreter_;
    interpreter_ = nullptr;

    // 销毁模块管理器
    delete module_manager_;
    module_manager_ = nullptr;

    // 销毁 Isolate 独立随机数状态。
    delete random_number_generator_;
    random_number_generator_ = nullptr;

    // 销毁句柄作用域实现
    delete handle_scope_implementer_;
    handle_scope_implementer_ = nullptr;

    delete factory_;
    factory_ = nullptr;

    klass_list_.Resize(0);

    // 清空单例引用
    py_none_object_ = Tagged<PyNone>::null();
    py_true_object_ = Tagged<PyBoolean>::null();
    py_false_object_ = Tagged<PyBoolean>::null();
    builtins_ = Tagged<PyObject>::null();

    // 销毁堆
    heap_->TearDown();
    delete heap_;
    heap_ = nullptr;

    // 销毁线程管理器
    delete thread_manager_;
    thread_manager_ = nullptr;
  } while (0);
  Exit();
}

}  // namespace saauso::internal
