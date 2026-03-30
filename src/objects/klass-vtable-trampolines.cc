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
void ThrowCompareUnsupported(Isolate* isolate,
                             Handle<PyObject> self,
                             Handle<PyObject> other,
                             const char* op) {
  auto self_name = PyObject::GetTypeName(self, isolate);
  auto other_name = PyObject::GetTypeName(other, isolate);
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "'%s' not supported between instances of '%s' and '%s'\n",
                      op, self_name->buffer(), other_name->buffer());
}
}  // namespace

MaybeHandle<PyObject> KlassVtableTrampolines::Add(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self, args,
                                          Handle<PyDict>::null(), ST(add, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Sub(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self, args,
                                          Handle<PyDict>::null(), ST(sub, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Mul(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self, args,
                                          Handle<PyDict>::null(), ST(mul, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Div(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self, args,
                                          Handle<PyDict>::null(), ST(div, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::FloorDiv(Isolate* isolate,
                                                       Handle<PyObject> self,
                                                       Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
          isolate, self, args, Handle<PyDict>::null(), ST(floor_div, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Mod(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self, args,
                                          Handle<PyDict>::null(), ST(mod, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

Maybe<uint64_t> KlassVtableTrampolines::Hash(Isolate* isolate,
                                             Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(hash, isolate))
           .ToHandle(&result)) {
    return kNullMaybe;
  }

  if (!IsPySmi(result)) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__hash__ method should return an integer");
    return kNullMaybe;
  }

  auto value = static_cast<uint64_t>(PySmi::ToInt(Handle<PySmi>::cast(result)));
  return Maybe<uint64_t>(value);
}

Maybe<bool> KlassVtableTrampolines::GetAttr(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> prop_name,
                                            bool is_try,
                                            Handle<PyObject>& out_prop_val) {
  // Python 中 __getattr__ 魔法方法不具备彻底重写默认属性查询流程的能力。
  // 因此这里还是转发到通用属性查询流程进行处理！
  return PyObjectKlass::Generic_GetAttr(isolate, self, prop_name, is_try,
                                        out_prop_val);
}

MaybeHandle<PyObject> KlassVtableTrampolines::SetAttr(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> property_name,
    Handle<PyObject> property_value) {
  Handle<PyTuple> args = PyTuple::New(isolate, 2);
  args->SetInternal(0, property_name);
  args->SetInternal(1, property_value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
          isolate, self, args, Handle<PyDict>::null(), ST(setattr, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Subscr(Isolate* isolate,
                                                     Handle<PyObject> self,
                                                     Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
          isolate, self, args, Handle<PyDict>::null(), ST(subscr, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::StoreSubscr(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> subscr,
    Handle<PyObject> value) {
  Handle<PyTuple> args = PyTuple::New(isolate, 2);
  args->SetInternal(0, subscr);
  args->SetInternal(1, value);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
           isolate, self, args, Handle<PyDict>::null(), ST(store_subscr, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object(), isolate);
}

MaybeHandle<PyObject> KlassVtableTrampolines::DeleteSubscr(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> subscr) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, subscr);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
           isolate, self, args, Handle<PyDict>::null(), ST(del_subscr, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object(), isolate);
}

Maybe<bool> KlassVtableTrampolines::Greater(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> other) {
  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(greater, isolate), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(isolate, self, other, ">");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
}

Maybe<bool> KlassVtableTrampolines::Less(Isolate* isolate,
                                         Handle<PyObject> self,
                                         Handle<PyObject> other) {
  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(less, isolate), callable));

  if (callable.is_null()) {
    ThrowCompareUnsupported(isolate, self, other, "<");
    return kNullMaybe;
  }

  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
}

Maybe<bool> KlassVtableTrampolines::Equal(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other) {
  if (self.is_identical_to(other)) {
    return Maybe<bool>(true);
  }

  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(equal, isolate), callable));

  if (callable.is_null()) {
    return Maybe<bool>(false);
  }

  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

  return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
}

Maybe<bool> KlassVtableTrampolines::NotEqual(Isolate* isolate,
                                             Handle<PyObject> self,
                                             Handle<PyObject> other) {
  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(not_equal, isolate), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::New(isolate, 1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
  }

  Maybe<bool> eq = Equal(isolate, self, other);
  if (eq.IsNothing()) {
    return kNullMaybe;
  }
  return Maybe<bool>(!eq.ToChecked());
}

Maybe<bool> KlassVtableTrampolines::GreaterEqual(Isolate* isolate,
                                                 Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(ge, isolate), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::New(isolate, 1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
  }

  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Greater(isolate, self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Equal(isolate, self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> KlassVtableTrampolines::LessEqual(Isolate* isolate,
                                              Handle<PyObject> self,
                                              Handle<PyObject> other) {
  Handle<PyObject> callable;
  RETURN_ON_EXCEPTION(isolate, Runtime_LookupPropertyInInstanceTypeMro(
                                   isolate, self, ST(le, isolate), callable));

  if (!callable.is_null()) {
    Handle<PyTuple> args = PyTuple::New(isolate, 1);
    args->SetInternal(0, other);

    Handle<PyObject> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, callable, self, args, Handle<PyDict>::null()));

    return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
  }

  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Less(isolate, self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Equal(isolate, self, other));

  return Maybe<bool>(lt || eq);
}

Maybe<bool> KlassVtableTrampolines::Contains(Isolate* isolate,
                                             Handle<PyObject> self,
                                             Handle<PyObject> other) {
  Handle<PyTuple> args = PyTuple::New(isolate, 1);
  args->SetInternal(0, other);

  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
          isolate, self, args, Handle<PyDict>::null(), ST(contains, isolate))
           .ToHandle(&result)) {
    return kNullMaybe;
  }
  return Maybe<bool>(Runtime_PyObjectIsTrue(isolate, result));
}

MaybeHandle<PyObject> KlassVtableTrampolines::Iter(Isolate* isolate,
                                                   Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(iter, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Next(Isolate* isolate,
                                                   Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(next, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Call(Isolate* isolate,
                                                   Handle<PyObject> self,
                                                   Handle<PyObject> receiver,
                                                   Handle<PyObject> args,
                                                   Handle<PyObject> kwargs) {
  Handle<PyObject> callable;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, callable,
      Runtime_GetPropertyInInstanceTypeMro(isolate, self, ST(call, isolate)));

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

MaybeHandle<PyObject> KlassVtableTrampolines::Len(Isolate* isolate,
                                                  Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(len, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Repr(Isolate* isolate,
                                                   Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(repr, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }

  if (!IsPyString(result)) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__repr__ returned non-string");
    return kNullMaybeHandle;
  }

  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::Str(Isolate* isolate,
                                                  Handle<PyObject> self) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(isolate, self,
                                          Handle<PyTuple>::null(),
                                          Handle<PyDict>::null(), ST(str, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }

  if (!IsPyString(result)) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__str__ returned non-string");
    return kNullMaybeHandle;
  }

  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Handle<PyObject> new_method;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, new_method,
      Runtime_GetPropertyInKlassMro(isolate, receiver_type->own_klass(),
                                    ST(new_instance, isolate)));

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, result,
                             Execution::Call(isolate, new_method, receiver_type,
                                             Handle<PyTuple>::cast(args),
                                             Handle<PyDict>::cast(kwargs)));

  return result;
}

MaybeHandle<PyObject> KlassVtableTrampolines::InitInstance(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Handle<PyObject> result;
  if (!Runtime_InvokeMagicOperationMethod(
           isolate, instance, Handle<PyTuple>::cast(args),
           Handle<PyDict>::cast(kwargs), ST(init_instance, isolate))
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

}  // namespace saauso::internal
