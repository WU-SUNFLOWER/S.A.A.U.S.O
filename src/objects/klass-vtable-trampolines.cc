// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass-vtable-trampolines.h"

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {
// 用于在不支持的比较运算上抛出 TypeError 异常，而不是直接退出进程。
void ThrowCompareUnsupported(Handle<PyObject> self,
                             Handle<PyObject> other,
                             const char* op) {
  auto self_name = PyObject::GetKlass(self)->name();
  auto other_name = PyObject::GetKlass(other)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "'%s' not supported between instances of '%s' and '%s'\n",
                      op, self_name->buffer(), other_name->buffer());
}
}  // namespace

MaybeHandle<PyObject> KlassVtableTrampolines::Add(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(add))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Sub(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(sub))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Mul(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(mul))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Div(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(div))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::FloorDiv(Handle<PyObject> self,
                                                       Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(floor_div))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Mod(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(mod))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

Maybe<uint64_t> KlassVtableTrampolines::Hash(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(hash))
           .ToHandle(&result)) {
    return kNullMaybe;
  }

  if (!IsPySmi(result)) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "__hash__ method should return an integer");
    return kNullMaybe;
  }

  auto value = static_cast<uint64_t>(PySmi::ToInt(Handle<PySmi>::cast(result)));
  return Maybe<uint64_t>(value);
}

Maybe<bool> KlassVtableTrampolines::GetAttr(Handle<PyObject> self,
                                            Handle<PyObject> prop_name,
                                            bool is_try,
                                            Handle<PyObject>& out_prop_val) {
  // Python 中 __getattr__ 魔法方法不具备彻底重写默认属性查询流程的能力。
  // 因此这里还是转发到通用属性查询流程进行处理！
  return PyObjectKlass::Generic_GetAttr(self, prop_name, is_try, out_prop_val);
}

MaybeHandle<PyObject> KlassVtableTrampolines::SetAttr(
    Handle<PyObject> self,
    Handle<PyObject> property_name,
    Handle<PyObject> property_value) {
  Handle<PyTuple> args = PyTuple::NewInstance(2);
  args->SetInternal(0, property_name);
  args->SetInternal(1, property_value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(setattr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Subscr(Handle<PyObject> self,
                                                     Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(subscr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::StoreSubscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr,
    Handle<PyObject> value) {
  auto* isolate = Isolate::Current();
  Handle<PyTuple> args = PyTuple::NewInstance(2);
  args->SetInternal(0, subscr);
  args->SetInternal(1, value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(store_subscr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> KlassVtableTrampolines::DeleteSubscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr) {
  auto* isolate = Isolate::Current();
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(del_subscr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

Maybe<bool> KlassVtableTrampolines::Greater(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(greater), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(self, other, ">");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> KlassVtableTrampolines::Less(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(less), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(self, other, "<");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> KlassVtableTrampolines::Equal(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  if (self.is_identical_to(other)) {
    return Maybe<bool>(true);
  }

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(equal), callable));

  if (callable.is_null()) {
    return Maybe<bool>(false);
  }

  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

Maybe<bool> KlassVtableTrampolines::NotEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(not_equal), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  Maybe<bool> eq = Equal(self, other);
  if (eq.IsNothing()) {
    return kNullMaybe;
  }
  return Maybe<bool>(!eq.ToChecked());
}

Maybe<bool> KlassVtableTrampolines::GreaterEqual(Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(ge), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Greater(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Equal(self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> KlassVtableTrampolines::LessEqual(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(le), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::NewInstance(1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(result));
  }

  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Less(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Equal(self, other));

  return Maybe<bool>(lt || eq);
}

Maybe<bool> KlassVtableTrampolines::Contains(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::NewInstance(1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, args, Handle<PyDict>::null(),
                                          ST(contains))
           .ToHandle(&result)) {
    return kNullMaybe;
  }
  return Maybe<bool>(Runtime_PyObjectIsTrue(result));
}

MaybeHandle<PyObject> KlassVtableTrampolines::Iter(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(iter))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Next(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(next))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Call(Isolate* isolate,
                                                   Handle<PyObject> self,
                                                   Handle<PyObject> host,
                                                   Handle<PyObject> args,
                                                   Handle<PyObject> kwargs) {
  Handle<PyObject> callable;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, callable,
      Runtime_GetPropertyInInstanceTypeMro(isolate, self, ST(call)));

  Handle<PyTuple> call_args =
      args.is_null() ? Handle<PyTuple>::null() : Handle<PyTuple>::cast(args);
  Handle<PyDict> call_kwargs =
      kwargs.is_null() ? Handle<PyDict>::null() : Handle<PyDict>::cast(kwargs);
  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, call_args, call_kwargs));
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Len(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(len))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Repr(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(repr))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Str(Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(self, Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(str))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::NewInstance(
    Isolate* isolate,
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Handle<PyObject> new_method;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, new_method,
      Runtime_GetPropertyInKlassMro(isolate, klass_self, ST(new_instance)));

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, new_method, klass_self->type_object(),
                      Handle<PyTuple>::cast(args),
                      Handle<PyDict>::cast(kwargs)));

  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::InitInstance(
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(instance, Handle<PyTuple>::cast(args),
                                          Handle<PyDict>::cast(kwargs),
                                          ST(init_instance))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

}  // namespace saauso::internal
