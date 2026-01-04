// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILD_BUILDFLAG_H_
#define SAAUSO_BUILD_BUILDFLAG_H_

// 魔法宏：直接展开参数
// 如果 IS_WIN 是 1，BUILDFLAG(IS_WIN) 就变成 (1)
// 如果 IS_WIN 是 0，BUILDFLAG(IS_WIN) 就变成 (0)
#define BUILDFLAG(x) (x)

#endif  // SAAUSO_BUILD_BUILDFLAG_H_
