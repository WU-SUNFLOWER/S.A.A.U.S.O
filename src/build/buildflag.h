#ifndef BUILD_BUILDFLAG_H_
#define BUILD_BUILDFLAG_H_

// 魔法宏：直接展开参数
// 如果 IS_WIN 是 1，BUILDFLAG(IS_WIN) 就变成 (1)
// 如果 IS_WIN 是 0，BUILDFLAG(IS_WIN) 就变成 (0)
#define BUILDFLAG(x) (x)

#endif  // BUILD_BUILDFLAG_H_
