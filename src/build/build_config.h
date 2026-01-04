// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILD_BUILD_CONFIG_H_
#define SAAUSO_BUILD_BUILD_CONFIG_H_

// 1. 先把所有平台默认设为 0 (False)
#define IS_WIN 0
#define IS_LINUX 0
#define IS_MAC 0

// 2. 根据编译器的预定义宏进行探测
#if defined(_WIN32)
#undef IS_WIN
#define IS_WIN 1
#elif defined(__APPLE__)
#undef IS_MAC
#define IS_MAC 1
#elif defined(__linux__)
#undef IS_LINUX
#define IS_LINUX 1
#else
#error "Please add support for your platform in build_config.h"
#endif

#endif  // SAAUSO_BUILD_BUILD_CONFIG_H_
