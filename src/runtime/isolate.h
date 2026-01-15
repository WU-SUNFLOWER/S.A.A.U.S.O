// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_ISOLATE_H_
#define SAAUSO_RUNTIME_ISOLATE_H_

#include "src/handles/tagged.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Heap;
class Klass;
class HandleScopeImplementer;
class Interpreter;
class StringTable;
class PyNone;
class PyBoolean;
class PyObjectKlass;
class NativeFunctionKlass;
class PySmiKlass;
class PyTypeObjectKlass;
class PyStringKlass;
class PyFunctionKlass;
class PyFloatKlass;
class PyBooleanKlass;
class PyNoneKlass;
class PyCodeObjectKlass;
class PyListKlass;
class PyDictKlass;
class FixedArrayKlass;
class MethodObjectKlass;

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

  Tagged<PyObjectKlass> py_object_klass() const { return py_object_klass_; }
  void set_py_object_klass(Tagged<PyObjectKlass> klass) {
    py_object_klass_ = klass;
  }

  Tagged<NativeFunctionKlass> native_function_klass() const {
    return native_function_klass_;
  }
  void set_native_function_klass(Tagged<NativeFunctionKlass> klass) {
    native_function_klass_ = klass;
  }

  Tagged<PySmiKlass> py_smi_klass() const { return py_smi_klass_; }
  void set_py_smi_klass(Tagged<PySmiKlass> klass) { py_smi_klass_ = klass; }

  Tagged<PyTypeObjectKlass> py_type_object_klass() const {
    return py_type_object_klass_;
  }
  void set_py_type_object_klass(Tagged<PyTypeObjectKlass> klass) {
    py_type_object_klass_ = klass;
  }

  Tagged<PyStringKlass> py_string_klass() const { return py_string_klass_; }
  void set_py_string_klass(Tagged<PyStringKlass> klass) {
    py_string_klass_ = klass;
  }

  Tagged<PyFunctionKlass> py_function_klass() const { return py_function_klass_; }
  void set_py_function_klass(Tagged<PyFunctionKlass> klass) {
    py_function_klass_ = klass;
  }

  Tagged<PyFloatKlass> py_float_klass() const { return py_float_klass_; }
  void set_py_float_klass(Tagged<PyFloatKlass> klass) { py_float_klass_ = klass; }

  Tagged<PyBooleanKlass> py_boolean_klass() const { return py_boolean_klass_; }
  void set_py_boolean_klass(Tagged<PyBooleanKlass> klass) {
    py_boolean_klass_ = klass;
  }

  Tagged<PyNoneKlass> py_none_klass() const { return py_none_klass_; }
  void set_py_none_klass(Tagged<PyNoneKlass> klass) { py_none_klass_ = klass; }

  Tagged<PyCodeObjectKlass> py_code_object_klass() const {
    return py_code_object_klass_;
  }
  void set_py_code_object_klass(Tagged<PyCodeObjectKlass> klass) {
    py_code_object_klass_ = klass;
  }

  Tagged<PyListKlass> py_list_klass() const { return py_list_klass_; }
  void set_py_list_klass(Tagged<PyListKlass> klass) { py_list_klass_ = klass; }

  Tagged<PyDictKlass> py_dict_klass() const { return py_dict_klass_; }
  void set_py_dict_klass(Tagged<PyDictKlass> klass) { py_dict_klass_ = klass; }

  Tagged<FixedArrayKlass> fixed_array_klass() const { return fixed_array_klass_; }
  void set_fixed_array_klass(Tagged<FixedArrayKlass> klass) {
    fixed_array_klass_ = klass;
  }

  Tagged<MethodObjectKlass> method_object_klass() const {
    return method_object_klass_;
  }
  void set_method_object_klass(Tagged<MethodObjectKlass> klass) {
    method_object_klass_ = klass;
  }

 private:
  Isolate() = default;
  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

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

  Tagged<PyObjectKlass> py_object_klass_;
  Tagged<NativeFunctionKlass> native_function_klass_;

  Tagged<PySmiKlass> py_smi_klass_;
  Tagged<PyTypeObjectKlass> py_type_object_klass_;
  Tagged<PyStringKlass> py_string_klass_;
  Tagged<PyFunctionKlass> py_function_klass_;
  Tagged<PyFloatKlass> py_float_klass_;
  Tagged<PyBooleanKlass> py_boolean_klass_;
  Tagged<PyNoneKlass> py_none_klass_;
  Tagged<PyCodeObjectKlass> py_code_object_klass_;
  Tagged<PyListKlass> py_list_klass_;
  Tagged<PyDictKlass> py_dict_klass_;
  Tagged<FixedArrayKlass> fixed_array_klass_;
  Tagged<MethodObjectKlass> method_object_klass_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_ISOLATE_H_
