// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_ISOLATE_H_
#define SAAUSO_RUNTIME_ISOLATE_H_

#include <thread>

#include "src/execution/exception-state.h"
#include "src/execution/isolate-klass-list.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/utils/vector.h"

namespace saauso::internal {

using ThreadId = std::thread::id;

class Heap;
class Klass;
class HandleScopeImplementer;
class Interpreter;
class ModuleManager;
class StringTable;

#define DECLARE_ISOLATE_KLASS_TYPES(Type, Klass, _) \
  class Type;                                       \
  class Klass;
ISOLATE_KLASS_LIST(DECLARE_ISOLATE_KLASS_TYPES)
#undef DECLARE_ISOLATE_KLASS_TYPES

// Isolate 代表了一个独立的 S.A.A.U.S.O 虚拟机实例。
// 它封装了所有虚拟机运行所需的全局状态（堆、句柄管理、解释器、类列表等），
// 从而允许在同一个进程中存在多个相互隔离的虚拟机实例。
// 这一设计借鉴了 V8 的 Isolate 概念。
class Isolate {
 public:
  // Isolate::Scope 是一个 RAII 辅助类，用于在当前线程中进入/退出指定的
  // Isolate。 在构造时调用 Enter()，析构时调用 Exit()。
  //
  // 用法示例：
  // void RunCode(Isolate* isolate) {
  //   Isolate::Scope scope(isolate); // 进入 Isolate，设置 Current()
  //   // 现在可以使用 Isolate::Current() 获取当前 isolate
  //   isolate->interpreter()->Run();
  // } // 自动退出 Isolate
  class Scope {
   public:
    explicit Scope(Isolate* isolate);
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    ~Scope();

   private:
    Isolate* isolate_{nullptr};
  };

  // Isolate::Locker 用于多线程环境下的同步。
  // 它确保在同一时刻只有一个线程可以访问该 Isolate（类似于 GIL 的作用范围，但在
  // Isolate 级别）。
  //
  // 用法示例：
  // void RunThreadSafe(Isolate* isolate) {
  //   Isolate::Locker lock(isolate); // 获取锁，如果被占用则阻塞
  //   Isolate::Scope scope(isolate); // 进入作用域
  //   // 安全地使用 isolate...
  // }
  class Locker {
   public:
    explicit Locker(Isolate* isolate);
    Locker(const Locker&) = delete;
    Locker& operator=(const Locker&) = delete;
    ~Locker();

   private:
    Isolate* isolate_{nullptr};
  };

  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

  // 创建一个新的 Isolate 实例。
  static Isolate* Create();
  // 销毁一个 Isolate 实例。
  static void Dispose(Isolate* isolate);
  static Isolate* New() { return Create(); }
  static void Delete(Isolate* isolate) { Dispose(isolate); }

  // 获取当前线程绑定的 Isolate 实例。
  static Isolate* Current();
  // 禁止直接设置 Current，必须通过 Scope 或 Enter/Exit 管理。
  static void SetCurrent(Isolate* isolate) = delete;

  // 进入 Isolate：将当前线程与此 Isolate 绑定。
  void Enter();
  // 退出 Isolate：解除当前线程与此 Isolate 的绑定。
  void Exit();

  // 获取虚拟机的核心组件
  Heap* heap() const { return heap_; }
  HandleScopeImplementer* handle_scope_implementer() const {
    return handle_scope_implementer_;
  }
  Vector<Tagged<Klass>>& klass_list() { return klass_list_; }
  Interpreter* interpreter() const { return interpreter_; }
  ModuleManager* module_manager() const { return module_manager_; }
  StringTable* string_table() const { return string_table_; }
  ExceptionState* exception_state() { return &exception_state_; }
  const ExceptionState* exception_state() const { return &exception_state_; }

  // 获取全局单例对象（None, True, False）
  Tagged<PyNone> py_none_object() const { return py_none_object_; }
  Tagged<PyBoolean> py_true_object() const { return py_true_object_; }
  Tagged<PyBoolean> py_false_object() const { return py_false_object_; }

  // 辅助函数：将 C++ bool 转换为 Python 的 True/False 对象
  static Tagged<PyBoolean> ToPyBoolean(bool condition);

#define DECLARE_ISOLATE_KLASS_ACCESSORS(_, Klass, slot)        \
  Tagged<Klass> slot##_klass() const { return slot##_klass_; } \
  void set_##slot##_klass(Tagged<Klass> klass) { slot##_klass_ = klass; }
  ISOLATE_KLASS_LIST(DECLARE_ISOLATE_KLASS_ACCESSORS)
#undef DECLARE_ISOLATE_KLASS_ACCESSORS

 private:
  Isolate() = default;

  void Init();
  void TearDown();
  void InitMetaArea();

  static ThreadId GetCurrentThreadId();
  void CheckThreadAccess() const;

  static thread_local Isolate* current_;

  Heap* heap_{nullptr};
  HandleScopeImplementer* handle_scope_implementer_{nullptr};
  Vector<Tagged<Klass>> klass_list_;
  Interpreter* interpreter_{nullptr};
  ModuleManager* module_manager_{nullptr};
  StringTable* string_table_{nullptr};
  ExceptionState exception_state_;

  Tagged<PyNone> py_none_object_;
  Tagged<PyBoolean> py_true_object_;
  Tagged<PyBoolean> py_false_object_;

#define DECLARE_ISOLATE_KLASS_FIELDS(_, Klass, slot) \
  Tagged<Klass> slot##_klass_;
  ISOLATE_KLASS_LIST(DECLARE_ISOLATE_KLASS_FIELDS)
#undef DECLARE_ISOLATE_KLASS_FIELDS

  ThreadId owner_thread_;
  int entry_count_{0};

  void* mutex_{nullptr};
  ThreadId locker_thread_;
  int locker_count_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_ISOLATE_H_
