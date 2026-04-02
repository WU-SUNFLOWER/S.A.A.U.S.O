// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-primitive.h"
#include "include/saauso-script.h"
#include "src/api/api-support.h"
#include "src/execution/isolate.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exec.h"

namespace saauso {

MaybeLocal<Script> Script::Compile(Isolate* isolate, Local<String> source) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);

  if (source.IsEmpty()) {
    return MaybeLocal<Script>();
  }

#if SAAUSO_ENABLE_CPYTHON_COMPILER
  i::Handle<internal::PyString> script =
      i::PyString::New(i_isolate, source->Value().c_str());
  return api::Utils::ToLocal<Script>(script);
#else
  i::HandleScope handle_scope(i_isolate);
  i::Runtime_ThrowError(
      i::ExceptionType::kRuntimeError,
      "Script::Compile requires CPython frontend compiler support");
  api::CapturePendingException(i_isolate);
  return MaybeLocal<Script>();
#endif
}

MaybeLocal<Value> Script::Run(Local<Context> context) {
  i::Isolate* internal_isolate = api::RequireCurrentIsolate();

  if (context.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> script_object = api::Utils::OpenHandle(this);
  if (script_object.is_null() || !i::IsPyString(script_object)) {
    return MaybeLocal<Value>();
  }

  i::Handle<i::PyObject> context_object = api::Utils::OpenHandle(context);
  if (context_object.is_null() || !i::IsPyDict(context_object)) {
    return MaybeLocal<Value>();
  }

  i::Tagged<i::PyString> source_string =
      i::Tagged<i::PyString>::cast(*script_object);
  i::EscapableHandleScope handle_scope(internal_isolate);
  i::Handle<i::PyDict> globals = i::Handle<i::PyDict>::cast(context_object);
  i::MaybeHandle<i::PyObject> maybe_result = i::Runtime_ExecutePythonSourceCode(
      internal_isolate,
      std::string_view(source_string->buffer(),
                       static_cast<size_t>(source_string->length())),
      globals, globals);
  i::Handle<i::PyObject> result;
  if (!maybe_result.ToHandle(&result)) {
    api::CapturePendingException(internal_isolate);
    return MaybeLocal<Value>();
  }
  i::Handle<i::PyObject> escaped = handle_scope.Escape(result);
  return api::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
