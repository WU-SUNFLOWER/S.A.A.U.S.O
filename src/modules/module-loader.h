// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_LOADER_H_
#define SAAUSO_MODULES_MODULE_LOADER_H_

#include <string>
#include <string_view>
#include <vector>

namespace saauso::internal {

// 文件系统中的模块/包定位信息。
//
// - origin：模块入口文件路径（module 为 name.py，package 为 __init__.py）。
// - is_package：是否为 package。
// - package_dir：当 is_package==true 时，package 目录路径（用于构造 __path__）。
struct ModuleLocation {
  std::string origin;
  bool is_package{false};
  std::string package_dir;
};

// ModuleLoader 只负责“基于文件系统”的模块/包查找与源码读取。
// 它不接触 sys.modules、不执行 Python 代码，也不依赖解释器语义。
class ModuleLoader final {
 public:
  ModuleLoader() = default;
  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;
  ~ModuleLoader() = default;

  // 在 search_paths 中查找 relative_parts 所表示的模块/包。
  //
  // 查找规则（MVP）：
  // 1) 优先查找 package：<base>/<relative_parts>/__init__.py
  // 2) 再查找 module：<base>/<relative_parts>.py
  //
  // relative_parts 的含义：相对于某个 search_path 的路径片段。
  // 例如导入 pkg.sub 时，当 base 为 package.__path__ 中的某项，relative_parts 应为 {"sub"}。
  ModuleLocation FindModuleLocation(const std::vector<std::string>& search_paths,
                                    const std::vector<std::string>& relative_parts) const;

  // 读取模块源码到 out。失败返回 false。
  bool ReadModuleSource(const ModuleLocation& location, std::string* out) const;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_LOADER_H_

