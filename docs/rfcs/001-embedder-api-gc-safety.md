# S.A.A.U.S.O Embedder API 架构重构方案：对齐 V8 HandleScope 与 GC 安全机制

**Author:** [Your Name/AI] (Google L5 VM Expert Persona)  
**Status:** Ready for Implementation  
**Target Assignee:** ChatGPT (Executor)  
**Reviewer:** Trae AI (Architect / Code Reviewer)  

---

## 1. 背景与缺陷分析 (Problem Statement)

当前 S.A.A.U.S.O Embedder API 的 `Local<T>` 设计存在严重的内存安全（Memory Safety）缺陷，无法与 Moving GC 正确协同。

**现状缺陷：**
1. **悬空指针 (Dangling Pointer) 风险**：当前的 `Local<T>` 间接持有一个分配在 C++ 堆上的 `ValueImpl` 对象，而 `ValueImpl` 内部以 `i::Tagged<i::PyObject>` 的形式保存了原始的虚拟机堆对象地址。
2. **脱离 Root Set**：`ValueImpl` 对象不在 GC 的根扫描集（Root Set）中。当 GC 触发 Copying 或 Compaction 导致对象移动时，`ValueImpl` 中的 `Tagged` 指针不会被更新，直接导致后续访问发生 Segfault。
3. **代理模式的性能开销**：每次通过 Embedder API 返回对象时，都会触发 `new Value()` 和 `new ValueImpl()`，造成不必要的系统堆分配开销（System Heap Allocation Overhead）。

## 2. 目标架构 (Target Architecture: V8 Alignment)

本次重构旨在将 Embedder API 彻底对齐 V8 引擎的 `Local`/`HandleScope` 架构。

**核心设计原则：**
1. **Zero-Overhead Abstraction (零开销抽象)**：`Local<T>` 在内存布局上只包含一个指针，直接指向内部 `i::HandleScope` 管理的槽位（Slot / `Address*`）。
2. **Opaque Classes (不透明类)**：API 暴露的 `Value`, `Object`, `String` 等类必须是**空类**（没有任何数据成员和虚表），严禁实例化。它们仅作为类型标记和方法载体。
3. **Implicit This-Pointer Casting (隐式 This 指针转换)**：在 API 的实现中，通过将 `this` 指针 `reinterpret_cast` 为内部的 `Address*`，从而以零开销获取内部的 `i::Handle`。
4. **Eager Allocation (即时分配)**：移除 `ValueKind::kHostString` 等延迟分配机制。宿主数据进入 API 边界时，必须立即在 VM 堆上分配为 `i::PyString` 等内部对象。

---

## 3. 分步执行计划 (Execution Plan)

请 ChatGPT 严格按照以下 4 个阶段进行代码修改，并确保每个阶段的编译通过。

### Phase 1: 改造公开头文件 (Public Headers Refactoring)
**目标文件：** `include/saauso-embedder-core.h`

1. **重构 `Local<T>`**：
   - 移除 `Local` 对 `ApiAccess` 等友元的依赖。
   - 内部成员仅保留一个 `T* val_`。
   ```cpp
   template <typename T>
   class Local {
    public:
     Local() : val_(nullptr) {}
     template <typename S>
     static Local<T> Cast(Local<S> that) {
       return Local<T>(reinterpret_cast<T*>(that.val_));
     }
     T* operator->() const { return val_; }
     T* operator*() const { return val_; }
     bool IsEmpty() const { return val_ == nullptr; }
    private:
     friend class Utils; // 用于内部与 API 的桥接
     explicit Local(T* val) : val_(val) {}
     T* val_;
   };
   ```

2. **清空 `Value` 及其子类的数据成员**：
   - 移除 `Value` 类中的 `impl_` 和 `isolate_` 成员。
   - 删除虚析构函数 `virtual ~Value() = default;`（Opaque 类不需要析构）。
   - 显式删除构造函数、拷贝构造和赋值操作符，防止用户在栈或堆上直接创建 `Value` 对象。
   ```cpp
   class Value {
    public:
     bool IsString() const;
     bool IsInteger() const;
     // ...
    private:
     Value() = delete;
     ~Value() = delete;
     Value(const Value&) = delete;
     void operator=(const Value&) = delete;
   };
   ```

### Phase 2: 移除 `ValueImpl` 并引入 `Utils` 桥接层
**目标文件：** `src/api/api-impl.h` (或新建 `src/api/api-utils.h`)

1. **彻底删除旧机制**：
   - 删除 `ValueKind` 枚举。
   - 删除 `ValueImpl` 结构体。
   - 删除 `ApiAccess` 相关的注册逻辑（`RegisterValue`, `SetValueImpl` 等）。
2. **实现 `Utils` 桥接类**：
   - 实现外部 `Local` 和内部 `i::Handle` 之间的零开销强转机制。
   ```cpp
   namespace saauso {
   namespace internal {
   class Utils {
    public:
     // Local -> Handle
     template <typename T>
     static inline i::Handle<i::PyObject> OpenHandle(const Local<T>& local) {
       return i::Handle<i::PyObject>(reinterpret_cast<i::Address*>(*local));
     }
     
     // Handle -> Local
     template <typename T>
     static inline Local<T> ToLocal(i::Handle<i::PyObject> handle) {
       return Local<T>(reinterpret_cast<T*>(handle.location()));
     }
   };
   }  // namespace internal
   }  // namespace saauso
   ```

### Phase 3: 重写宿主数据包装器 (Eager Allocation)
**目标文件：** `src/api/api-impl.h` & `src/api/api-support.cc`

1. **重写 `WrapHostString` 等工厂函数**：
   - 以前是创建一个 C++ 堆上的 `ValueImpl` 缓存 `std::string`，现在必须**立即**调用 VM 内部的 `Factory` 分配对象。
   ```cpp
   // 以 WrapHostString 为例
   template <typename T>
   Local<T> WrapHostString(Isolate* isolate, std::string value) {
     if (isolate == nullptr) return Local<T>();
     i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
     
     // 1. 必须有 HandleScope 保护
     i::HandleScope scope(i_isolate);
     
     // 2. 立即在 VM 堆上分配 PyString
     i::Handle<i::PyString> internal_str = 
         i_isolate->factory()->NewStringFromAscii(value.c_str());
         
     // 3. 将内部 Handle 的地址转移到外部 Local (注意 Escape 逻辑，取决于实际实现)
     // 简化版：
     return internal::Utils::ToLocal<T>(internal_str); 
   }
   ```
   *(注：执行此阶段时，需注意 `WrapHost*` 的返回值需要能够逃逸出当前的临时 `HandleScope`，可利用 `i::EscapableHandleScope` 进行处理。)*

### Phase 4: 重写 API 类的实现方法 (Method Rewriting)
**目标文件：** `src/api/api-value.cc`, `src/api/api-objects.cc` 等

1. **利用 `this` 指针强转获取内部对象**：
   - 以前是通过 `GetValueImpl(this)` 读取代理对象。现在直接将 `this` 强转为槽位指针。
   ```cpp
   // src/api/api-value.cc
   #include "src/api/api-utils.h" // 或 api-impl.h

   bool Value::IsString() const {
     // 1. Opaque this pointer trick (V8 经典做法)
     i::Address* slot = reinterpret_cast<i::Address*>(const_cast<Value*>(this));
     i::Handle<i::PyObject> obj(slot);
     
     // 2. 内部逻辑调用
     return i::IsPyString(*obj);
   }

   bool Value::ToString(std::string* out) const {
     if (out == nullptr) return false;
     
     i::Address* slot = reinterpret_cast<i::Address*>(const_cast<Value*>(this));
     i::Handle<i::PyObject> obj(slot);
     
     if (!i::IsPyString(*obj)) return false;
     
     i::Tagged<i::PyString> py_string = i::Tagged<i::PyString>::cast(*obj);
     *out = std::string(py_string->buffer(), py_string->length());
     return true;
   }
   ```

---

## 4. 验收标准与 CR 清单 (Acceptance Criteria & Review Checklist)

ChatGPT 提交重构代码后，我（Reviewer）将重点审查以下事项：
- [ ] **ABI 纯净度**：`include/saauso-embedder-core.h` 中的 `Value` 等类必须绝对干净，不能包含任何 `virtual` 函数和私有成员变量（除了 `Local` 里的 `val_`）。
- [ ] **GC 安全性**：全仓搜索 `ValueImpl` 和 `ApiAccess::RegisterValue`，必须被彻底移除。所有的对象生命周期完全移交给 `i::HandleScope`。
- [ ] **性能指标**：`Utils::OpenHandle` 和 `Utils::ToLocal` 必须是 `inline` 的 `reinterpret_cast`，不能包含任何条件分支或内存分配。
- [ ] **编译状态**：所有的单元测试（特别是 `embedder_ut`）必须能够 Clean Build 并 Pass。

---
**致 Executor (ChatGPT):** 
请严格按照上述 4 个 Phase 进行代码替换。如果你在 `EscapableHandleScope` 的逃逸逻辑或内部 `Factory` API 的签名上遇到不确定的地方，请优先查阅 `src/handles/handle-scopes.h` 和 `src/heap/factory.h`。完成修改后，请通知我进行 Code Review。