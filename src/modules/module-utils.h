// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_UTILS_H_
#define SAAUSO_MODULES_MODULE_UTILS_H_

#include <string_view>

#include "include/saauso-maybe.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class PyDict;
class PyList;
class PyObject;
class PyString;

class ModuleUtils final : public AllStatic {
 public:
  static bool IsValidModuleName(Handle<PyString> fullname);

  static Maybe<bool> IsPackageModule(Isolate* isolate, Handle<PyObject> module);

  // 1.对于普通模块（单个.py文件），__path__属性通常不存在，执行后out为空。
  // 2.对于包（包含__init__.py文件的目录），__path__的主要作用是指定从哪些目录中查找包的子模块。
  //   一般情况下，Python会自动为包对象设置一个初始的__path__属性。其为一个列表，包含该包所在的目录路径。
  // 3.本API的返回值若为空，则表示执行查询过程中发生了异常，需要调用方继续向上传播异常。
  static Maybe<void> GetPackagePathList(Isolate* isolate,
                                        Handle<PyObject> module,
                                        Handle<PyList>& out);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_UTILS_H_
