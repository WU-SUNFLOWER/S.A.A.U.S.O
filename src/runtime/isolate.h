// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_ISOLATE_H_
#define SAAUSO_RUNTIME_ISOLATE_H_

#include "src/handles/tagged.h"
#include "src/runtime/isolate-klass-list.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Heap;
class Klass;
class HandleScopeImplementer;
class Interpreter;
class StringTable;

#define DECLARE_ISOLATE_KLASS_TYPES(Type, Klass, _) \
  class Type;                                       \
  class Klass;
ISOLATE_KLASS_LIST(DECLARE_ISOLATE_KLASS_TYPES)
#undef DECLARE_ISOLATE_KLASS_TYPES

class Isolate {
 public:
  class Scope {
   public:
    explicit Scope(Isolate* isolate);
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    ~Scope();

   private:
    Isolate* previous_{nullptr};
  };

  static Isolate* Create();
  static void Dispose(Isolate* isolate);

  static Isolate* Current();
  static void SetCurrent(Isolate* isolate);

  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

  Heap* heap() const { return heap_; }
  HandleScopeImplementer* handle_scope_implementer() const {
    return handle_scope_implementer_;
  }
  Vector<Klass*>& klass_list() { return klass_list_; }
  Interpreter* interpreter() const { return interpreter_; }
  StringTable* string_table() const { return string_table_; }

  Tagged<PyNone> py_none_object() const { return py_none_object_; }
  Tagged<PyBoolean> py_true_object() const { return py_true_object_; }
  Tagged<PyBoolean> py_false_object() const { return py_false_object_; }

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

  static thread_local Isolate* current_;

  Heap* heap_{nullptr};
  HandleScopeImplementer* handle_scope_implementer_{nullptr};
  Vector<Klass*> klass_list_;
  Interpreter* interpreter_{nullptr};
  StringTable* string_table_{nullptr};

  Tagged<PyNone> py_none_object_;
  Tagged<PyBoolean> py_true_object_;
  Tagged<PyBoolean> py_false_object_;

#define DECLARE_ISOLATE_KLASS_FIELDS(_, Klass, slot) \
  Tagged<Klass> slot##_klass_;
  ISOLATE_KLASS_LIST(DECLARE_ISOLATE_KLASS_FIELDS)
#undef DECLARE_ISOLATE_KLASS_FIELDS
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_ISOLATE_H_
