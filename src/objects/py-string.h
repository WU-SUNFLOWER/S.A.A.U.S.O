// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_STRING_H_
#define SAAUSO_OBJECTS_STRING_H_

#include <cstdint>
#include <string>

#include "src/handles/handles.h"
#include "src/objects/py-object.h"
#include "src/utils/string-search.h"

namespace saauso::internal {

class PyFloat;
class PySmi;

class PyString : public PyObject {
 public:
  static constexpr int kNotFound = StringSearch::kNotFound;

  static Handle<PyString> NewInstance(int64_t str_length,
                                      bool in_meta_space = false);
  static Handle<PyString> NewInstance(const char* source,
                                      bool in_meta_space = false);
  static Handle<PyString> NewInstance(const char* source,
                                      int64_t str_length,
                                      bool in_meta_space = false);

  static Handle<PyString> Clone(Handle<PyString> other,
                                bool in_meta_space = false);

  static Handle<PyString> FromPySmi(Tagged<PySmi> smi);
  static Handle<PyString> FromInt(int64_t n);
  static Handle<PyString> FromPyFloat(Handle<PyFloat> py_float);
  static Handle<PyString> FromDouble(double n);

  static Tagged<PyString> cast(Tagged<PyObject> object);

  static size_t ComputeObjectSize(int64_t str_length);

  void Set(int64_t index, char value);
  char Get(int64_t index) const;

  uint64_t GetHash();
  bool HasHashCache() const;

  bool IsLessThan(Tagged<PyString> other);
  bool IsEqualTo(Tagged<PyString> other);
  bool IsGreaterThan(Tagged<PyString> other);

  int64_t IndexOf(Handle<PyString> pattern) const;
  int64_t IndexOf(Handle<PyString> pattern, int64_t begin, int64_t end) const;

  int64_t LastIndexOf(Handle<PyString> pattern) const;
  int64_t LastIndexOf(Handle<PyString> pattern,
                      int64_t begin,
                      int64_t end) const;

  bool Contains(Handle<PyString> pattern) const;

  bool IsEmpty() const { return length_ == 0; }

  int64_t length() const { return length_; }

  const char* buffer() const { return reinterpret_cast<const char*>(this + 1); }

  // 获取可写 buffer，用于在分配后快速填充字符串内容。
  // 注意：该接口仅供虚拟机内部使用，调用方必须保证写入不越界。
  char* writable_buffer() { return reinterpret_cast<char*>(this + 1); }

  char front() const {
    assert(!IsEmpty());
    return Get(0);
  }

  char back() const {
    assert(!IsEmpty());
    return Get(length_ - 1);
  }

  std::string ToStdString() const;
  const char* ToCString() const { return buffer(); }

  static Handle<PyString> Slice(Handle<PyString> self, int64_t from);
  static Handle<PyString> Slice(Handle<PyString> self,
                                int64_t from,
                                int64_t to);
  static Handle<PyString> Append(Handle<PyString> self, Handle<PyString> other);

 private:
  friend class PyStringKlass;
  friend class Factory;

  int64_t length_{0};
  uint64_t hash_{0};
};

/*
TODO: 字符串优化

```C++
// 预留枚举，MVP版本可能只有 kSequential
enum class StringRepresentation {
  kSequential, // 内联 buffer (当前实现)
  kSliced,     // 引用其他 string (未来实现)
  kCons        // 拼接节点 (未来实现)
};

class PyString : public PyObject {
 public:
  // ... 通用接口 ...
  int64_t length() const { return length_; }
  uint64_t GetHash();

  // 【关键点】：不要直接暴露 buffer_ 成员或无脑假设 buffer 在后面
  // 改为提供一个“尝试获取”的接口
  // 如果是 SlicedString，这个函数未来会返回 nullptr 或者触发 Flatten
  const char* TryGetRawBuffer() const;

  // 强制获取 buffer，如果当前是 Sliced/Cons，会触发内存规整（Flatten），
  // 将复杂结构转化为内联结构。这是 V8 处理复杂字符串转 C-String 的标准做法。
  const char* FlattenAndGetBuffer();

 protected:
  // PyString 只包含元数据
  int64_t length_{0};
  uint64_t hash_{0};
};

// 当前的 MVP 实现本质上是 PySeqString
class PySeqString : public PyString {
 public:
  // 只有 SeqString 知道数据就在屁股后面
  const char* raw_data() const {
    return reinterpret_cast<const char*>(this + 1);
  }

  // 写数据时使用
  char* raw_data_rw() {
    return reinterpret_cast<char*>(this + 1);
  }

  static Handle<PySeqString> New(const char* data, int64_t len);
};
```

```C++
class PyStringKlass : public Klass {
  // ...
  // 添加一个标记位，或者针对不同 String 子类创建不同的 Klass 实例
  // 方案 A: 所有 String 共用一个 Klass，但在 Klass 里查 flag
  // 方案 B (推荐): PySeqStringKlass, PySlicedStringKlass 继承自 PyStringKlass

  virtual size_t InstanceSize(Tagged<PyObject> obj) override {
    // 这是一个虚函数（或通过 switch type 分发）
    // 如果是 SeqString: return sizeof(PySeqString) + len
    // 如果是 SlicedString: return sizeof(PySlicedString) (固定大小)
  }
};
```

*/

}  // namespace saauso::internal

#endif
