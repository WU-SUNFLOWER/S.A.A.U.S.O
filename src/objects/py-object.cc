// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "include/saauso-maybe.h"
#include "py-object.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/cell-klass.h"
#include "src/objects/cell.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/mark-word.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/utils/utils.h"

namespace saauso::internal {

MarkWord PyObject::GetMarkWord(Tagged<PyObject> object) {
  assert(IsHeapObject(object));
  return object->mark_word_;
}

void PyObject::SetMapWordForwarded(Tagged<PyObject> object,
                                   Tagged<PyObject> target) {
  object->mark_word_ = MarkWord::FromForwardingAddress(target);
}

Tagged<Klass> PyObject::ResolveObjectKlass(Tagged<PyObject> object,
                                           Isolate* isolate) {
  // 特化：Smi使用PySmiKlass，使得它表现得像一个标准的Python对象
  if (IsPySmi(object)) {
    return PySmiKlass::GetInstance(isolate);
  }
  return GetHeapKlassUnchecked(object);
}

Tagged<Klass> PyObject::ResolveObjectKlass(Handle<PyObject> object,
                                           Isolate* isolate) {
  return ResolveObjectKlass(*object, isolate);
}

Tagged<Klass> PyObject::GetHeapKlassUnchecked(Tagged<PyObject> object) {
  assert(IsHeapObject(object));
  assert(!object->mark_word_.ToKlass().is_null());
  return object->mark_word_.ToKlass();
}

void PyObject::SetKlass(Tagged<PyObject> object, Tagged<Klass> klass) {
  assert(!klass.is_null());
  assert(IsHeapObject(object));
  object->mark_word_ = MarkWord::FromKlass(klass);
}

void PyObject::SetKlass(Handle<PyObject> object, Tagged<Klass> klass) {
  SetKlass(*object, klass);
}

Handle<PyString> PyObject::GetTypeName(Handle<PyObject> object,
                                       Isolate* isolate) {
  return GetTypeName(*object, isolate);
}

Handle<PyString> PyObject::GetTypeName(Tagged<PyObject> object,
                                       Isolate* isolate) {
  return ResolveObjectKlass(object, isolate)->name();
}

Handle<PyDict> PyObject::GetProperties(Handle<PyObject> object) {
  return GetProperties(*object);
}

Handle<PyDict> PyObject::GetProperties(Tagged<PyObject> object) {
  if (!IsHeapObject(object) || object->properties_.is_null()) {
    return Handle<PyDict>::null();
  }
  return handle(Tagged<PyDict>::cast(object->properties_));
}

void PyObject::SetProperties(Tagged<PyObject> object,
                             Tagged<PyDict> properties) {
  // properties不能是smi！
  assert(properties.is_null() || IsHeapObject(properties));

  object->properties_ = properties;
  WRITE_BARRIER(object, &object->properties_, properties);
}

///////////////////////////////////////////////////////////////////////////
// 多态虚方法入口 开始

MaybeHandle<PyObject> PyObject::Repr(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().repr_);
  return klass->vtable().repr_(isolate, self);
}

MaybeHandle<PyObject> PyObject::Str(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().str_);
  return klass->vtable().str_(isolate, self);
}

// python virtual function
MaybeHandle<PyObject> PyObject::Add(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() +
                                           PySmi::cast(*other).value()));
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().add_);
  return klass->vtable().add_(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::Sub(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() -
                                           PySmi::cast(*other).value()));
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().sub_);
  return klass->vtable().sub_(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::Mul(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() *
                                           PySmi::cast(*other).value()));
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().mul_);
  return klass->vtable().mul_(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::Div(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().div_);
  return klass->vtable().div_(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::FloorDiv(Isolate* isolate,
                                         Handle<PyObject> self,
                                         Handle<PyObject> other) {
  int64_t other_value;
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(self) && IsPySmi(other) &&
      (other_value = PySmi::cast(*other).value()) != 0) {
    int64_t self_value = PySmi::cast(*self).value();
    return Handle<PyObject>(
        PySmi::FromInt(PythonFloorDivide(self_value, other_value)));
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  auto* floor_div = klass->vtable().floor_div_;
  if (floor_div == nullptr) [[unlikely]] {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "unsupported operand type(s) for //: '%s' and '%s'",
                        GetTypeName(self, isolate)->buffer(),
                        GetTypeName(other, isolate)->buffer());
    return kNullMaybeHandle;
  }
  return floor_div(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::Mod(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(self) && IsPySmi(other)) {
    auto result =
        PythonMod(PySmi::cast(*self).value(), PySmi::cast(*other).value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().mod_);
  return klass->vtable().mod_(isolate, self, other);
}

MaybeHandle<PyObject> PyObject::Len(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().len_);
  return klass->vtable().len_(isolate, self);
}

MaybeHandle<PyBoolean> PyObject::Greater(Isolate* isolate,
                                         Handle<PyObject> self,
                                         Handle<PyObject> other) {
  bool result;
  if (!GreaterBool(isolate, self, other).To(&result)) {
    return kNullMaybeHandle;
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(result));
}

Maybe<bool> PyObject::GreaterBool(Isolate* isolate,
                                  Handle<PyObject> self,
                                  Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Maybe<bool>(PySmi::cast(*self).value() >
                       PySmi::cast(*other).value());
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().greater_);
  return klass->vtable().greater_(isolate, self, other);
}

MaybeHandle<PyBoolean> PyObject::Less(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> other) {
  Maybe<bool> mb = LessBool(isolate, self, other);
  if (mb.IsNothing()) {
    return MaybeHandle<PyBoolean>();
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(mb.ToChecked()));
}

Maybe<bool> PyObject::LessBool(Isolate* isolate,
                               Handle<PyObject> self,
                               Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Maybe<bool>(PySmi::cast(*self).value() <
                       PySmi::cast(*other).value());
  }

  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().less_);
  return klass->vtable().less_(isolate, self, other);
}

MaybeHandle<PyBoolean> PyObject::Equal(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> other) {
  Maybe<bool> mb = EqualBool(isolate, self, other);
  if (mb.IsNothing()) {
    return MaybeHandle<PyBoolean>();
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(mb.ToChecked()));
}

Maybe<bool> PyObject::EqualBool(Isolate* isolate,
                                Handle<PyObject> self,
                                Handle<PyObject> other) {
  if (self.is_identical_to(other)) {
    return Maybe<bool>(true);
  }
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().equal_);
  return klass->vtable().equal_(isolate, self, other);
}

MaybeHandle<PyBoolean> PyObject::NotEqual(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other) {
  Maybe<bool> mb = NotEqualBool(isolate, self, other);
  if (mb.IsNothing()) {
    return MaybeHandle<PyBoolean>();
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(mb.ToChecked()));
}

Maybe<bool> PyObject::NotEqualBool(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other) {
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Maybe<bool>(PySmi::cast(*self).value() !=
                       PySmi::cast(*other).value());
  }
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().not_equal_);
  return klass->vtable().not_equal_(isolate, self, other);
}

MaybeHandle<PyBoolean> PyObject::GreaterEqual(Isolate* isolate,
                                              Handle<PyObject> self,
                                              Handle<PyObject> other) {
  Maybe<bool> mb = GreaterEqualBool(isolate, self, other);
  if (mb.IsNothing()) {
    return MaybeHandle<PyBoolean>();
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(mb.ToChecked()));
}

Maybe<bool> PyObject::GreaterEqualBool(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> other) {
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Maybe<bool>(PySmi::cast(*self).value() >=
                       PySmi::cast(*other).value());
  }
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().ge_);
  return klass->vtable().ge_(isolate, self, other);
}

MaybeHandle<PyBoolean> PyObject::LessEqual(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> other) {
  Maybe<bool> mb = LessEqualBool(isolate, self, other);
  if (mb.IsNothing()) {
    return MaybeHandle<PyBoolean>();
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(mb.ToChecked()));
}

Maybe<bool> PyObject::LessEqualBool(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> other) {
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Maybe<bool>(PySmi::cast(*self).value() <=
                       PySmi::cast(*other).value());
  }
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().le_);
  return klass->vtable().le_(isolate, self, other);
}

Maybe<bool> PyObject::LookupAttr(Isolate* isolate,
                                 Handle<PyObject> self,
                                 Handle<PyObject> attr_name,
                                 Handle<PyObject>& out) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  auto* getattr = klass->vtable().getattr_;
  assert(getattr != nullptr);

  out = Handle<PyObject>::null();

  bool found;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             getattr(isolate, self, attr_name, true, out));

  assert(!found && out.is_null() || found && !out.is_null());
  return Maybe<bool>(found);
}

MaybeHandle<PyObject> PyObject::GetAttr(Isolate* isolate,
                                        Handle<PyObject> self,
                                        Handle<PyObject> attr_name) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  auto* getattr = klass->vtable().getattr_;
  assert(getattr != nullptr);

  Handle<PyObject> result;
  RETURN_ON_EXCEPTION(isolate,
                      getattr(isolate, self, attr_name, false, result));

  // 非try模式下调用getattr，如果没查到结果会直接抛异常，不可能返回一个空结果
  assert(!result.is_null());
  return result;
}

MaybeHandle<PyObject> PyObject::GetAttrForCall(Isolate* isolate,
                                               Handle<PyObject> self,
                                               Handle<PyObject> attr_name,
                                               Handle<PyObject>& self_or_null) {
  self_or_null = Handle<PyObject>::null();
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);

  // Fast Path:
  // 对于一般对象，直接查询对象方法对应的裸的PyFunction对象，绕开临时生成
  // MethodObject对象的操作。
  if (klass->vtable().getattr_ == &PyObjectKlass::Generic_GetAttr) {
    return PyObjectKlass::Generic_GetAttrForCall(isolate, self, attr_name,
                                                 self_or_null);
  }

  // Fallback
  Handle<PyObject> value;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, value, GetAttr(isolate, self, attr_name));

  if (IsMethodObject(value)) {
    auto method = Handle<MethodObject>::cast(value);
    self_or_null = method->owner(isolate);
    return method->func(isolate);
  }
  return value;
}

MaybeHandle<PyObject> PyObject::SetAttr(Isolate* isolate,
                                        Handle<PyObject> self,
                                        Handle<PyObject> attr_name,
                                        Handle<PyObject> attr_value) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().setattr_);
  return klass->vtable().setattr_(isolate, self, attr_name, attr_value);
}

MaybeHandle<PyObject> PyObject::Subscr(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> subscr_name) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().subscr_);
  return klass->vtable().subscr_(isolate, self, subscr_name);
}

MaybeHandle<PyObject> PyObject::StoreSubscr(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> subscr_name,
                                            Handle<PyObject> subscr_value) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().store_subscr_);
  return klass->vtable().store_subscr_(isolate, self, subscr_name,
                                       subscr_value);
}

MaybeHandle<PyObject> PyObject::DeletSubscr(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> subscr_name) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().del_subscr_);
  return klass->vtable().del_subscr_(isolate, self, subscr_name);
}

MaybeHandle<PyBoolean> PyObject::Contains(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> target) {
  bool result;
  if (!ContainsBool(isolate, self, target).To(&result)) {
    return kNullMaybeHandle;
  }
  return MaybeHandle<PyBoolean>(isolate->factory()->ToPyBoolean(result));
}

Maybe<bool> PyObject::ContainsBool(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> target) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().contains_);
  return klass->vtable().contains_(isolate, self, target);
}

MaybeHandle<PyObject> PyObject::Iter(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().iter_);
  return klass->vtable().iter_(isolate, self);
}

MaybeHandle<PyObject> PyObject::Next(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().next_);
  return klass->vtable().next_(isolate, self);
}

Maybe<uint64_t> PyObject::Hash(Isolate* isolate, Handle<PyObject> self) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  assert(klass->vtable().hash_);
  return klass->vtable().hash_(isolate, self);
}

MaybeHandle<PyObject> PyObject::Call(Isolate* isolate,
                                     Handle<PyObject> self,
                                     Handle<PyObject> receiver,
                                     Handle<PyObject> args,
                                     Handle<PyObject> kwargs) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  auto* call_method = klass->vtable().call_;
  return call_method(isolate, self, receiver, args, kwargs);
}

// python virtual function
size_t PyObject::GetInstanceSize(Tagged<PyObject> self) {
  return GetHeapKlassUnchecked(self)->vtable().instance_size_(self);
}

// python virtual function
void PyObject::Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  v->VisitPointer(&self->properties_);

  Tagged<Klass> klass = GetHeapKlassUnchecked(self);
  assert(klass->vtable().iterate_);
  klass->vtable().iterate_(self, v);
}

}  // namespace saauso::internal
