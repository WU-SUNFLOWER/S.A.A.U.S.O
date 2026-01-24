// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/isolate.h"

#include <mutex>
#include <thread>
#include <vector>

#include "include/saauso.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object-klass.h"
#include "src/runtime/string-table.h"


namespace saauso::internal {

thread_local Isolate* Isolate::current_ = nullptr;
thread_local std::vector<Isolate*> g_entered_isolates;

Isolate::Scope::Scope(Isolate* isolate) : isolate_(isolate) {
  if (isolate_ != nullptr) {
    isolate_->Enter();
  }
}

Isolate::Scope::~Scope() {
  if (isolate_ != nullptr) {
    isolate_->Exit();
  }
}

Isolate* Isolate::Create() {
  if (!saauso::Saauso::IsInitialized()) {
    // 错误：S.A.A.U.S.O 虚拟机全局环境未初始化。
    // 在创建 Isolate 之前，必须先调用 saauso::Saauso::Initialize()。
    assert(0);
  }
  auto* isolate = new Isolate();
  isolate->Init();
  return isolate;
}

void Isolate::Dispose(Isolate* isolate) {
  if (isolate == nullptr) {
    return;
  }
  if (isolate->entry_count_ != 0) {
    // 错误：试图销毁一个正在被使用的 Isolate。
    // 必须确保所有线程都已退出该 Isolate（即 entry_count_ 为 0）才能销毁它。
    assert(0);
  }
  isolate->TearDown();
  delete isolate;
}

Isolate* Isolate::Current() {
  return current_;
}

ThreadId Isolate::GetCurrentThreadId() {
  return std::this_thread::get_id();
}

Isolate::Locker::Locker(Isolate* isolate) : isolate_(isolate) {
  if (isolate_ == nullptr) {
    return;
  }
  reinterpret_cast<std::recursive_mutex*>(isolate_->mutex_)->lock();
  ThreadId tid = GetCurrentThreadId();
  if (isolate_->locker_thread_ == ThreadId{}) {
    isolate_->locker_thread_ = tid;
  }
  if (isolate_->locker_thread_ != tid) {
    // 错误：锁状态异常。
    // 当前线程获得了互斥锁，但 isolate 内部记录的 locker_thread_ 却是其他线程
    // ID。 这通常意味着锁的实现或状态维护出现了严重的逻辑错误。
    assert(0);
  }
  ++isolate_->locker_count_;
}

Isolate::Locker::~Locker() {
  if (isolate_ == nullptr) {
    return;
  }
  if (isolate_->entry_count_ != 0) {
    // 错误：试图在仍有 Scope 进入的情况下释放锁。
    // 必须先退出所有 Scope (Exit)，才能释放 Locker。
    // 即 Scope 的生命周期必须完全包含在 Locker 的生命周期内。
    assert(0);
  }
  if (isolate_->locker_thread_ != GetCurrentThreadId() ||
      isolate_->locker_count_ <= 0) {
    // 错误：解锁线程不匹配或锁计数异常。
    // 只有持有锁的线程才能释放锁，且锁计数必须大于 0。
    assert(0);
  }
  --isolate_->locker_count_;
  if (isolate_->locker_count_ == 0) {
    isolate_->locker_thread_ = ThreadId{};
    if (isolate_->entry_count_ == 0) {
      isolate_->owner_thread_ = ThreadId{};
    }
  }
  reinterpret_cast<std::recursive_mutex*>(isolate_->mutex_)->unlock();
}

void Isolate::CheckThreadAccess() const {
  ThreadId tid = GetCurrentThreadId();
  if (owner_thread_ == ThreadId{}) {
    return;
  }
  if (owner_thread_ != tid) {
    // 错误：线程访问违规。
    // 当前线程试图访问一个已被其他线程拥有的 Isolate，且未通过 Locker/Enter
    // 机制获得授权。
    assert(0);
  }
}

void Isolate::Enter() {
  ThreadId tid = GetCurrentThreadId();
  // 如果当前 Isolate 没有所有者，则将其分配给当前线程
  if (owner_thread_ == ThreadId{}) {
    owner_thread_ = tid;
  }
  // 如果当前线程不是所有者
  if (owner_thread_ != tid) {
    // 检查是否有锁（Locker），并且是持有锁的线程在重入
    if (!(locker_thread_ == tid && locker_count_ > 0 && entry_count_ == 0)) {
      // 错误：非法进入 Isolate。
      // 当前 Isolate 已被其他线程拥有（owner_thread_ != tid），
      // 且当前线程没有持有锁（Locker），或者状态不满足重入条件。
      // 多线程访问必须先获取 Locker。
      assert(0);
    }
    owner_thread_ = tid;
  }
  // 将之前的 current_ Isolate 压栈，支持嵌套进入不同的
  // Isolate（虽然实际场景很少见）
  g_entered_isolates.push_back(current_);
  current_ = this;
  ++entry_count_;
}

void Isolate::Exit() {
  ThreadId tid = GetCurrentThreadId();
  // 必须是所有者线程，且 current_ 必须是自己，且 entry_count_ 必须大于 0
  if (owner_thread_ != tid || current_ != this || entry_count_ <= 0 ||
      g_entered_isolates.empty()) {
    // 错误：非法退出 Isolate。
    // 可能原因：
    // 1. 当前线程不是 Isolate 的所有者。
    // 2. 退出顺序错误（未按栈顺序退出，current_ != this）。
    // 3. 进入计数已经为 0（重复退出）。
    assert(0);
  }
  --entry_count_;
  // 恢复之前的 current_ Isolate
  current_ = g_entered_isolates.back();
  g_entered_isolates.pop_back();
  // 如果完全退出了（entry_count_ == 0）且没有锁，则释放所有权
  if (entry_count_ == 0 && locker_count_ == 0) {
    owner_thread_ = ThreadId{};
  }
}

Tagged<PyBoolean> Isolate::ToPyBoolean(bool condition) {
  Isolate* isolate = Isolate::Current();
  return condition ? isolate->py_true_object_ : isolate->py_false_object_;
}

void Isolate::Init() {
  // 初始化互斥锁
  mutex_ = new std::recursive_mutex();

  // 初始化堆（Heap）
  heap_ = new Heap(this);
  heap_->Setup();

  // 初始化句柄作用域实现
  handle_scope_implementer_ = new HandleScopeImplementer();

  // 进入当前 Isolate 作用域，以便后续的初始化操作（如分配对象）能找到正确的
  // Isolate
  Scope isolate_scope(this);
  HandleScope scope;

  // 初始化元数据区域（Klasses, Singletons 等）
  InitMetaArea();

  // 初始化解释器
  interpreter_ = new Interpreter();
}

void Isolate::InitMetaArea() {
  // 1. 预初始化所有 Klass（设置 vtable，调用 PreInitialize）
#define PREINIT_PY_KLASS(_, Klass, __) \
  do {                                 \
    auto klass = Klass::GetInstance(); \
    klass->InitializeVTable();         \
    klass->PreInitialize();            \
  } while (false);
  ISOLATE_KLASS_LIST(PREINIT_PY_KLASS)
#undef PREINIT_PY_KLASS

  // 2. 初始化字符串表
  string_table_ = new StringTable();

  // 3. 创建全局单例对象（None, True, False）
  // 这些对象必须在其他逻辑使用它们之前创建好
  py_none_object_ = PyNone::NewInstance();
  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);

  // 4. 正式初始化所有 Klass（调用 Initialize）
#define INIT_PY_KLASS(_, Klass, __) Klass::GetInstance()->Initialize();
  ISOLATE_KLASS_LIST(INIT_PY_KLASS)
#undef INIT_PY_KLASS
}

void Isolate::TearDown() {
  Scope isolate_scope(this);

  // 反向操作：先销毁 Klass
#define FINALIZE_PY_KLASS(_, Klass, __) Klass::GetInstance()->Finalize();
  ISOLATE_KLASS_LIST(FINALIZE_PY_KLASS)
#undef FINALIZE_PY_KLASS

  // 销毁字符串表
  delete string_table_;
  string_table_ = nullptr;

  // 销毁解释器
  delete interpreter_;
  interpreter_ = nullptr;

  // 销毁句柄作用域实现
  delete handle_scope_implementer_;
  handle_scope_implementer_ = nullptr;

  klass_list_.Resize(0);

  // 清空单例引用
  py_none_object_ = Tagged<PyNone>::Null();
  py_true_object_ = Tagged<PyBoolean>::Null();
  py_false_object_ = Tagged<PyBoolean>::Null();

  // 销毁堆
  heap_->TearDown();
  delete heap_;
  heap_ = nullptr;

  // 销毁互斥锁
  delete reinterpret_cast<std::recursive_mutex*>(mutex_);
  mutex_ = nullptr;
}

}  // namespace saauso::internal
