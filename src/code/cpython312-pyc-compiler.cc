// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/cpython312-pyc-compiler.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>

#include "src/build/build_config.h"
#include "src/build/buildflag.h"

#if BUILDFLAG(IS_WIN)
#if defined(_DEBUG)
#define SAAUSO_INTERNAL_RESTORE_DEBUG 1
#undef _DEBUG
#endif
#include "third_party/cpython312/include_win/Python.h"
#include "third_party/cpython312/include_win/marshal.h"
#if defined(SAAUSO_INTERNAL_RESTORE_DEBUG)
#define _DEBUG 1
#undef SAAUSO_INTERNAL_RESTORE_DEBUG
#endif
#endif

#if BUILDFLAG(IS_LINUX)
#include "third_party/cpython312/include_linux/Python.h"
#include "third_party/cpython312/include_linux/marshal.h"
#endif

namespace saauso::internal {

namespace {

void PutInt32LE(std::vector<uint8_t>& out, int32_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

void FinalizeEmbeddedPython312Runtime() {
  if (!Py_IsInitialized()) {
    return;
  }
  Py_FinalizeEx();
}

void EnsurePythonInitialized() {
  static std::once_flag once;
  std::call_once(once, []() {
    PyPreConfig preconfig;
    PyPreConfig_InitIsolatedConfig(&preconfig);
    preconfig.allocator = PYMEM_ALLOCATOR_MALLOC;

    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
      Py_ExitStatusException(status);
    }

    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.configure_c_stdio = 0;
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
      Py_ExitStatusException(status);
    }
    std::atexit(FinalizeEmbeddedPython312Runtime);
  });
}

}  // namespace

std::vector<uint8_t> CompilePythonSourceToPycBytes312(
    std::string_view source,
    std::string_view filename) {
  EnsurePythonInitialized();

  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* code_obj = Py_CompileStringExFlags(std::string(source).c_str(),
                                               std::string(filename).c_str(),
                                               Py_file_input, nullptr, -1);
  if (code_obj == nullptr) {
    PyErr_Print();
    PyGILState_Release(gstate);
    std::exit(1);
  }

  PyObject* marshalled =
      PyMarshal_WriteObjectToString(code_obj, Py_MARSHAL_VERSION);
  Py_DECREF(code_obj);
  if (marshalled == nullptr) {
    PyErr_Print();
    PyGILState_Release(gstate);
    std::exit(1);
  }

  char* payload = nullptr;
  Py_ssize_t payload_size = 0;
  if (PyBytes_AsStringAndSize(marshalled, &payload, &payload_size) != 0) {
    PyErr_Print();
    Py_DECREF(marshalled);
    PyGILState_Release(gstate);
    std::exit(1);
  }

  std::vector<uint8_t> out;
  out.reserve(16 + static_cast<size_t>(payload_size));
  PutInt32LE(out, static_cast<int32_t>(PyImport_GetMagicNumber()));
  PutInt32LE(out, 0);
  PutInt32LE(out, 0);
  PutInt32LE(out, static_cast<int32_t>(source.size()));
  out.insert(out.end(), payload, payload + payload_size);

  Py_DECREF(marshalled);
  PyGILState_Release(gstate);
  return out;
}

}  // namespace saauso::internal
