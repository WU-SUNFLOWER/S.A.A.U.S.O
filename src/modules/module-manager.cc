// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-manager.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"
#include "src/utils/file-utils.h"

namespace saauso::internal {

namespace {

Handle<PyModule> InitSysModule(Isolate* isolate, ModuleManager* manager) {
  EscapableHandleScope scope;

  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyObject> module_obj(module);
  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);

  PyDict::Put(module_dict, PyString::NewInstance("__name__"),
              PyString::NewInstance("sys"));
  PyDict::Put(module_dict, PyString::NewInstance("__package__"),
              PyString::NewInstance(""));
  PyDict::Put(module_dict, PyString::NewInstance("modules"),
              manager->modules());
  PyDict::Put(module_dict, PyString::NewInstance("path"), manager->path());
  PyDict::Put(module_dict, PyString::NewInstance("version"),
              PyString::NewInstance("3.12 (saauso mvp)"));

  return scope.Escape(module);
}

std::string ToStdString(Handle<PyString> s) {
  if (s.is_null()) {
    return std::string();
  }
  return std::string(s->buffer(), static_cast<size_t>(s->length()));
}

std::vector<std::string> SplitModuleName(std::string_view name) {
  std::vector<std::string> parts;
  size_t start = 0;
  while (start <= name.size()) {
    size_t dot = name.find('.', start);
    if (dot == std::string_view::npos) {
      dot = name.size();
    }
    if (dot == start) {
      return {};
    }
    parts.emplace_back(name.substr(start, dot - start));
    if (dot == name.size()) {
      break;
    }
    start = dot + 1;
  }
  return parts;
}

std::string JoinModuleName(const std::vector<std::string>& parts,
                           size_t count) {
  std::string out;
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      out.push_back('.');
    }
    out.append(parts[i]);
  }
  return out;
}

struct ModuleLocation {
  std::string origin;
  bool is_package{false};
  std::string package_dir;
};

std::vector<std::string> ExtractSearchPaths(Handle<PyList> path_list) {
  std::vector<std::string> result;
  if (path_list.is_null()) {
    return result;
  }
  result.reserve(static_cast<size_t>(path_list->length()));
  for (int64_t i = 0; i < path_list->length(); ++i) {
    Handle<PyObject> elem = path_list->Get(i);
    if (elem.is_null()) {
      continue;
    }
    if (!IsPyString(elem)) {
      std::fprintf(stderr, "TypeError: sys.path items must be strings\n");
      std::exit(1);
    }
    result.emplace_back(ToStdString(Handle<PyString>::cast(elem)));
  }
  return result;
}

ModuleLocation FindModuleLocation(const std::vector<std::string>& search_paths,
                                  const std::vector<std::string>& parts) {
  ModuleLocation result;

  std::filesystem::path relative;
  for (const auto& part : parts) {
    relative /= std::filesystem::path(part);
  }

  for (const auto& base : search_paths) {
    std::filesystem::path base_path(base);

    std::filesystem::path package_init = base_path / relative / "__init__.py";
    if (FileExists(package_init.string())) {
      result.origin = NormalizePath(package_init.string());
      result.is_package = true;
      result.package_dir = NormalizePath((base_path / relative).string());
      return result;
    }

    std::filesystem::path module_py = base_path / relative;
    module_py += ".py";
    if (FileExists(module_py.string())) {
      result.origin = NormalizePath(module_py.string());
      result.is_package = false;
      return result;
    }
  }

  return result;
}

bool IsPackageModule(Handle<PyObject> module) {
  Handle<PyDict> dict = PyObject::GetProperties(module);
  if (dict.is_null()) {
    return false;
  }
  Handle<PyObject> path = dict->Get(PyString::NewInstance("__path__"));
  return !path.is_null() && IsPyList(path);
}

Handle<PyList> GetPackagePathListOrDie(Handle<PyObject> module) {
  Handle<PyDict> dict = PyObject::GetProperties(module);
  Handle<PyObject> path_obj = dict->Get(PyString::NewInstance("__path__"));
  if (path_obj.is_null() || !IsPyList(path_obj)) [[unlikely]] {
    std::fprintf(stderr, "ImportError: parent is not a package\n");
    std::exit(1);
  }
  return Handle<PyList>::cast(path_obj);
}

std::string ParentModuleNameOrEmpty(std::string_view name) {
  size_t dot = name.rfind('.');
  if (dot == std::string_view::npos) {
    return std::string();
  }
  return std::string(name.substr(0, dot));
}

std::string ResolvePackageFromGlobals(Handle<PyDict> globals) {
  if (globals.is_null()) {
    return std::string();
  }

  Handle<PyObject> package_obj =
      globals->Get(PyString::NewInstance("__package__"));
  if (!package_obj.is_null()) {
    if (!IsPyString(package_obj)) {
      std::fprintf(stderr, "TypeError: __package__ must be a string\n");
      std::exit(1);
    }
    return ToStdString(Handle<PyString>::cast(package_obj));
  }

  Handle<PyObject> name_obj = globals->Get(PyString::NewInstance("__name__"));
  if (name_obj.is_null() || !IsPyString(name_obj)) {
    std::fprintf(stderr, "ImportError: missing __name__ in globals\n");
    std::exit(1);
  }

  std::string name = ToStdString(Handle<PyString>::cast(name_obj));
  bool is_package = !globals->Get(PyString::NewInstance("__path__")).is_null();
  if (is_package) {
    return name;
  }
  return ParentModuleNameOrEmpty(name);
}

std::string ResolveRelativeImportName(std::string_view name,
                                      int64_t level,
                                      Handle<PyDict> globals) {
  if (level <= 0) {
    return std::string(name);
  }

  std::string base = ResolvePackageFromGlobals(globals);
  if (base.empty()) {
    std::fprintf(stderr,
                 "ImportError: attempted relative import with no known parent "
                 "package\n");
    std::exit(1);
  }

  for (int i = 1; i < level; ++i) {
    std::string parent = ParentModuleNameOrEmpty(base);
    if (parent.empty()) {
      std::fprintf(
          stderr,
          "ImportError: attempted relative import beyond top-level package\n");
      std::exit(1);
    }
    base = std::move(parent);
  }

  if (name.empty()) {
    return base;
  }
  std::string fullname = base;
  fullname.push_back('.');
  fullname.append(name);
  return fullname;
}

// 校验并解析 import 请求，返回最终的绝对模块名。
std::string ValidateAndResolveFullName(Handle<PyString> name,
                                       int64_t level,
                                       Handle<PyDict> globals) {
  assert(level >= 0);

  std::string name_str = ToStdString(name);

  // 当 level==0 时，name 必须非空。
  // 当 level>0 时，name 可以为空，例如 `from . import x`。
  if (level == 0 && name_str.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: empty module name\n");
    std::exit(1);
  }

  return ResolveRelativeImportName(name_str, level, globals);
}

// 根据 CPython import 语义：当 fromlist 为空且 import 的 name 为 dotted-name
// 时， 返回顶层包；否则返回最后导入的模块对象。
Handle<PyObject> ApplyImportReturnSemantics(
    Handle<PyDict> modules_dict,
    const std::vector<std::string>& parts,
    Handle<PyTuple> fromlist,
    Handle<PyObject> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  if (!has_fromlist && parts.size() > 1) {
    Handle<PyString> top_name = PyString::NewInstance(
        parts[0].c_str(), static_cast<int64_t>(parts[0].size()));
    return modules_dict->Get(top_name);
  }
  return last_module;
}

// 在 parent 模块上挂载子模块引用（parent.child = child）。
// 这让 `import pkg.sub; pkg.sub` 在 Python 层可用。
void BindChildToParent(Handle<PyDict> modules_dict,
                       std::string_view parent_fullname,
                       std::string_view child_short_name,
                       Handle<PyObject> child) {
  Handle<PyString> parent_name = PyString::NewInstance(
      parent_fullname.data(), static_cast<int64_t>(parent_fullname.size()));
  Handle<PyObject> parent = modules_dict->Get(parent_name);
  if (parent.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: missing parent module '%.*s'\n",
                 static_cast<int>(parent_fullname.size()),
                 parent_fullname.data());
    std::exit(1);
  }

  Handle<PyDict> parent_dict = PyObject::GetProperties(parent);
  PyDict::Put(
      parent_dict,
      PyString::NewInstance(child_short_name.data(),
                            static_cast<int64_t>(child_short_name.size())),
      child);
}

// 初始化 module 的 namespace（复用 properties_）。
// 该函数只负责写入模块元数据，不负责执行代码。
void InitializeModuleDict(Handle<PyModule> module,
                          Handle<PyString> fullname,
                          const std::vector<std::string>& parts,
                          size_t part_index,
                          const ModuleLocation& loc) {
  Handle<PyObject> module_obj(module);
  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);

  PyDict::Put(module_dict, PyString::NewInstance("__name__"), fullname);

  if (loc.is_package) {
    PyDict::Put(module_dict, PyString::NewInstance("__package__"), fullname);
  } else if (part_index == 0) {
    PyDict::Put(module_dict, PyString::NewInstance("__package__"),
                PyString::NewInstance(""));
  } else {
    std::string parent_name = JoinModuleName(parts, part_index);
    PyDict::Put(
        module_dict, PyString::NewInstance("__package__"),
        PyString::NewInstance(parent_name.c_str(),
                              static_cast<int64_t>(parent_name.size())));
  }

  PyDict::Put(module_dict, PyString::NewInstance("__file__"),
              PyString::NewInstance(loc.origin.c_str(),
                                    static_cast<int64_t>(loc.origin.size())));

  PyDict::Put(module_dict, ST(class),
              PyObject::GetKlass(module_obj)->type_object());

  if (loc.is_package) {
    Handle<PyList> pkg_path = PyList::NewInstance();
    PyList::Append(pkg_path, PyString::NewInstance(
                                 loc.package_dir.c_str(),
                                 static_cast<int64_t>(loc.package_dir.size())));
    PyDict::Put(module_dict, PyString::NewInstance("__path__"), pkg_path);
  }
}

// 加载一个非 builtin 的源码模块（.py 或 package/__init__.py）。
// 注意：必须先插入 sys.modules 再执行模块体，以支持循环导入。
Handle<PyObject> LoadSourceModule(
    ModuleManager* manager,
    Handle<PyDict> modules_dict,
    Handle<PyString> fullname,
    const std::vector<std::string>& parts,
    size_t part_index,
    const std::vector<std::string>& search_paths) {
  std::vector<std::string> relative_parts;
  if (part_index == 0) {
    relative_parts = std::vector<std::string>(parts.begin(), parts.begin() + 1);
  } else {
    relative_parts = std::vector<std::string>{parts[part_index]};
  }

  ModuleLocation loc = FindModuleLocation(search_paths, relative_parts);
  if (loc.origin.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: No module named '%s'\n",
                 ToStdString(fullname).c_str());
    std::exit(1);
  }

  std::string source;
  if (!ReadFileToString(loc.origin, &source)) {
    std::fprintf(stderr, "ImportError: cannot read '%s'\n", loc.origin.c_str());
    std::exit(1);
  }

  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, parts, part_index, loc);

  Handle<PyObject> module_obj(module);
  PyDict::Put(modules_dict, fullname, module_obj);

  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);
  Handle<PyFunction> boilerplate =
      Compiler::CompileSource(manager->isolate(), std::string_view(source),
                              std::string_view(loc.origin));
  boilerplate->set_func_globals(module_dict);
  manager->isolate()->interpreter()->CallPython(
      boilerplate, Handle<PyObject>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), module_dict);

  return module_obj;
}

// 加载 dotted-name 的某一段模块（可能是 builtin，也可能是用户源码）。
Handle<PyObject> LoadModulePart(ModuleManager* manager,
                                Handle<PyDict> modules_dict,
                                Handle<PyString> fullname,
                                const std::vector<std::string>& parts,
                                size_t part_index,
                                const std::vector<std::string>& search_paths) {
  std::string fullname_str = ToStdString(fullname);
  ModuleManager::BuiltinModuleInitFunc builtin_init =
      manager->FindBuiltinModule(fullname_str);
  if (builtin_init != nullptr) {
    Handle<PyModule> module = builtin_init(manager->isolate(), manager);
    return Handle<PyObject>(module);
  }

  return LoadSourceModule(manager, modules_dict, fullname, parts, part_index,
                          search_paths);
}

// 导入一个绝对 dotted-name（fullname 已是绝对名），并返回最后导入的 module
// 对象。
Handle<PyObject> ImportDottedName(ModuleManager* manager,
                                  Handle<PyDict> modules_dict,
                                  const std::vector<std::string>& parts) {
  Handle<PyObject> last_module;

  for (size_t i = 0; i < parts.size(); ++i) {
    std::string part_name = JoinModuleName(parts, i + 1);
    Handle<PyString> part_name_obj = PyString::NewInstance(
        part_name.c_str(), static_cast<int64_t>(part_name.size()));

    Handle<PyObject> cached = modules_dict->Get(part_name_obj);
    if (!cached.is_null()) {
      last_module = cached;
    } else {
      std::vector<std::string> search_paths;
      if (i == 0) {
        search_paths = ExtractSearchPaths(manager->path());
      } else {
        search_paths = ExtractSearchPaths(GetPackagePathListOrDie(last_module));
      }

      Handle<PyObject> loaded = LoadModulePart(
          manager, modules_dict, part_name_obj, parts, i, search_paths);

      PyDict::Put(modules_dict, part_name_obj, loaded);
      last_module = loaded;

      if (i > 0) {
        std::string parent_name = JoinModuleName(parts, i);
        BindChildToParent(modules_dict, parent_name, parts[i], loaded);
      }
    }

    if (i + 1 < parts.size() && !IsPackageModule(last_module)) {
      std::fprintf(stderr, "ImportError: '%s' is not a package\n",
                   part_name.c_str());
      std::exit(1);
    }
  }

  return last_module;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////////

ModuleManager::ModuleManager(Isolate* isolate) : isolate_(isolate) {
  InitializeSysState();
  RegisterBuiltinModules();
}

ModuleManager::~ModuleManager() = default;

void ModuleManager::Iterate(ObjectVisitor* v) {
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&modules_));
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&path_));
}

Handle<PyDict> ModuleManager::modules() const {
  return handle(modules_tagged());
}

Tagged<PyDict> ModuleManager::modules_tagged() const {
  return Tagged<PyDict>::cast(modules_);
}

Handle<PyList> ModuleManager::path() const {
  return handle(path_tagged());
}

Tagged<PyList> ModuleManager::path_tagged() const {
  return Tagged<PyList>::cast(path_);
}

void ModuleManager::InitializeSysState() {
  HandleScope scope;

  Handle<PyDict> modules = PyDict::NewInstance();
  modules_ = *modules;

  Handle<PyList> path = PyList::NewInstance();
  PyList::Append(path, PyString::NewInstance("."));
  path_ = *path;
}

void ModuleManager::RegisterBuiltinModule(std::string_view name,
                                          BuiltinModuleInitFunc init) {
  BuiltinModuleEntry entry;
  entry.name = std::string(name);
  entry.init = init;
  builtin_modules_.push_back(std::move(entry));
}

ModuleManager::BuiltinModuleInitFunc ModuleManager::FindBuiltinModule(
    std::string_view name) const {
  for (const auto& entry : builtin_modules_) {
    if (entry.name == name) {
      return entry.init;
    }
  }
  return nullptr;
}

void ModuleManager::RegisterBuiltinModules() {
  RegisterBuiltinModule("sys", &InitSysModule);
}

Handle<PyObject> ModuleManager::ImportModule(Handle<PyString> name,
                                             Handle<PyTuple> fromlist,
                                             int64_t level,
                                             Handle<PyDict> globals) {
  EscapableHandleScope scope;

  std::string fullname = ValidateAndResolveFullName(name, level, globals);
  std::vector<std::string> parts = SplitModuleName(fullname);
  if (parts.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: invalid module name '%s'\n",
                 fullname.c_str());
    std::exit(1);
  }

  Handle<PyDict> modules_dict = modules();
  Handle<PyObject> last_module = ImportDottedName(this, modules_dict, parts);
  Handle<PyObject> result =
      ApplyImportReturnSemantics(modules_dict, parts, fromlist, last_module);
  (void)globals;
  return scope.Escape(result);
}

}  // namespace saauso::internal
