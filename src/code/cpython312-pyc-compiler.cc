// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/cpython312-pyc-compiler.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
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

void FinalizeEmbeddedPython312Runtime();

namespace {

void PutInt32LE(std::vector<uint8_t>& out, int32_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

std::mutex g_python312_lifecycle_mutex;
bool g_python312_atexit_registered = false;

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
bool PathExists(const std::filesystem::path& p) {
  std::error_code ec;
  return std::filesystem::exists(p, ec);
}

bool LooksLikePython312Home(const std::filesystem::path& home) {
  return PathExists(home / "Lib" / "encodings" / "__init__.py") ||
         PathExists(home / "Lib" / "encodings" / "__init__.pyc") ||
         PathExists(home / "Lib" / "encodings");
}

#if BUILDFLAG(IS_WIN)
constexpr const char* kPython312HomeDirName = "python_home_win";
#else
constexpr const char* kPython312HomeDirName = "python_home_linux";
#endif

std::optional<std::filesystem::path> FindPython312Home() {
  if (const char* env = std::getenv("SAAUSO_PYTHON312_HOME")) {
    std::filesystem::path p(env);
    if (LooksLikePython312Home(p)) {
      return p;
    }
  }

  std::error_code ec;
  std::filesystem::path cur = std::filesystem::current_path(ec);
  if (ec) {
    return std::nullopt;
  }
  for (int i = 0; i < 12; i++) {
    std::filesystem::path p1 =
        cur / "third_party" / "cpython312" / kPython312HomeDirName;
    if (LooksLikePython312Home(p1)) {
      return p1;
    }
    if (cur.has_parent_path()) {
      cur = cur.parent_path();
    } else {
      break;
    }
  }

  return std::nullopt;
}

void AppendModuleSearchPath(PyConfig* config, const std::filesystem::path& p) {
  if (!PathExists(p)) {
    return;
  }
  wchar_t* w = Py_DecodeLocale(p.string().c_str(), nullptr);
  if (w == nullptr) {
    std::abort();
  }
  PyStatus status = PyWideStringList_Append(&config->module_search_paths, w);
  PyMem_RawFree(w);
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }
}

void ConfigurePython312HomeAndPath(PyConfig* config) {
  std::optional<std::filesystem::path> home = FindPython312Home();
  if (!home) {
    return;
  }

  PyStatus status =
      PyConfig_SetBytesString(config, &config->home, home->string().c_str());
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }

  config->module_search_paths_set = 1;

  AppendModuleSearchPath(config, *home / "Lib");
}
#endif

void EnsurePythonInitialized() {
  std::lock_guard<std::mutex> lock(g_python312_lifecycle_mutex);
  if (Py_IsInitialized()) {
    return;
  }
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
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
  ConfigurePython312HomeAndPath(&config);
#endif
  status = Py_InitializeFromConfig(&config);
  PyConfig_Clear(&config);
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }
  if (!g_python312_atexit_registered) {
    std::atexit(FinalizeEmbeddedPython312Runtime);
    g_python312_atexit_registered = true;
  }
}

}  // namespace

void FinalizeEmbeddedPython312Runtime() {
  std::lock_guard<std::mutex> lock(g_python312_lifecycle_mutex);
  if (!Py_IsInitialized()) {
    return;
  }

  PyGILState_STATE gstate = PyGILState_Ensure();
  PyGC_Collect();
  PyGC_Collect();
  PyGC_Collect();
  PyType_ClearCache();
  PyGILState_Release(gstate);

  Py_FinalizeEx();
}

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
