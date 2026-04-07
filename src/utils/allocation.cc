// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/allocation.h"

#include <cassert>
#include <cstdint>

#include "build/build_config.h"
#include "build/buildflag.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX)
#include <sys/mman.h>
#include <sys/types.h>
#endif  // BUILDFLAG(IS_LINUX)

namespace saauso::internal {

namespace {

#if BUILDFLAG(IS_LINUX)
// Constants used for mmap.
constexpr int kMmapFd = -1;
constexpr int kMmapFdOffset = 0;
#endif  // BUILDFLAG(IS_LINUX)

bool IsPowerOfTwo(size_t n) {
  return (n & (n - 1)) == 0;
}

uintptr_t RoundUp(uintptr_t addr, size_t alignment) {
  return (addr + alignment - 1) & ~(alignment - 1);
}

}  // namespace

void* VirtualMemory::address() {
  assert(IsReserved());
  return address_;
}

#if BUILDFLAG(IS_WIN)
VirtualMemory::VirtualMemory(size_t size) {
  Reserve(size, 0);
}

VirtualMemory::VirtualMemory(size_t size, size_t alignment) {
  Reserve(size, alignment);
}

void VirtualMemory::Reserve(size_t size, size_t alignment) {
  size_ = size;
  address_ = nullptr;

  void* base;
  uintptr_t base_addr;
  uintptr_t aligned_addr;
  BOOL free_result;
  size_t padded_size;
  constexpr int kMaxAttempts = 3;

  // 0. 如果没有对齐需求，那么直接申请一块虚拟内存并返回
  if (alignment == 0) {
    address_ = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    goto ret;
  }

  // 如果有对齐要求，对齐长度必须是2的幂次
  assert(IsPowerOfTwo(alignment));

  // 1. 先尝试直接按原大小分配，如果碰巧对齐了，直接返回
  base = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
  if (base == nullptr) [[unlikely]] {
    goto ret;
  }

  base_addr = reinterpret_cast<uintptr_t>(base);
  aligned_addr = RoundUp(base_addr, alignment);
  if (aligned_addr == base_addr) {
    address_ = base;
    goto ret;
  }

  // 没有对齐，释放掉
  free_result = VirtualFree(base, 0, MEM_RELEASE);
  if (!free_result) [[unlikely]] {
    goto ret_with_free_failed;
  }

  // 2. 采用 V8 的 "Padding 机制" 保证分配区间内必存在对齐地址
  // 增加 alignment 作为 padding，保证其中必有一段连续的 size 满足对齐要求
  padded_size = size + alignment;

  for (int i = 0; i < kMaxAttempts; ++i) {
    base = VirtualAlloc(nullptr, padded_size, MEM_RESERVE, PAGE_NOACCESS);
    if (base == nullptr) [[unlikely]] {
      goto ret;
    }

    base_addr = reinterpret_cast<uintptr_t>(base);
    aligned_addr = RoundUp(base_addr, alignment);

    // 核心：释放整个 Padded 内存块。
    // Windows VirtualFree(MEM_RELEASE) 必须传分配时的首地址，且不能部分释放。
    free_result = VirtualFree(base, 0, MEM_RELEASE);
    if (!free_result) [[unlikely]] {
      goto ret_with_free_failed;
    }

    // 立即在算好的对齐地址上重新精确分配 size 大小。
    // 这里存在极小概率的 Race Condition（释放后瞬间被其他线程抢占），因此需要
    // kMaxAttempts 循环重试。
    base = VirtualAlloc(reinterpret_cast<void*>(aligned_addr), size,
                        MEM_RESERVE, PAGE_NOACCESS);
    if (base != nullptr) [[likely]] {
      address_ = base;
      goto ret;
    }
  }

ret:
  return;

ret_with_free_failed:
  if (!free_result) {
    address_ = nullptr;
    std::printf("fail to call VirtualFree!\n");
    std::exit(1);
  }
  return;
}

VirtualMemory::~VirtualMemory() {
  if (IsReserved()) {
    if (VirtualFree(address(), 0, MEM_RELEASE) == 0) {
      address_ = nullptr;
    }
  }
}

bool VirtualMemory::IsReserved() {
  return address_ != nullptr;
}

bool VirtualMemory::Commit(void* address, size_t size, bool is_executable) {
  int prot = is_executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
  if (VirtualAlloc(address, size, MEM_COMMIT, prot) == nullptr) {
    return false;
  }
  return true;
}

bool VirtualMemory::Uncommit(void* address, size_t size) {
  assert(IsReserved());
  return VirtualFree(address, size, MEM_DECOMMIT) != FALSE;
}
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX)
VirtualMemory::VirtualMemory(size_t size) {
  Reserve(size, 0);
}

VirtualMemory::VirtualMemory(size_t size, size_t alignment) {
  Reserve(size, alignment);
}

void VirtualMemory::Reserve(size_t size, size_t alignment) {
  size_ = size;
  address_ = MAP_FAILED;

  void* base;
  uintptr_t base_addr;

  void* reservation;
  uintptr_t reservation_addr;
  uintptr_t aligned_addr;
  size_t prefix_size;
  size_t suffix_size;

  // 0. 如果没有对齐需求，那么直接申请一块虚拟内存并返回
  if (alignment == 0) {
    address_ = mmap(nullptr, size, PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, kMmapFd,
                    kMmapFdOffset);
    goto ret;
  }

  // 如果有对齐要求，对齐长度必须是2的幂次
  assert(IsPowerOfTwo(alignment));

  // 1. 先尝试精准分配原大小，如果碰巧对齐，可省去后续的 2 次 munmap
  base =
      mmap(nullptr, size, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, kMmapFd, kMmapFdOffset);
  if (base == MAP_FAILED) [[unlikely]] {
    goto ret;
  }

  base_addr = reinterpret_cast<uintptr_t>(base);
  if (RoundUp(base_addr, alignment) == base_addr) {
    address_ = base;
    goto ret;
  }

  // 没有对齐，释放掉
  munmap(base, size);

  // 2. 采用 Linux 典型的 Padded Allocation
  // mmap 必定返回页对齐(通常 4KB)的地址，因此最大 misalignment 是 alignment -
  // page_size。 为了不强依赖外部 page_size 常量，这里直接用 size + alignment
  // 作为 padding 也是完全安全且正确的。
  reservation =
      mmap(nullptr, size + alignment, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, kMmapFd, kMmapFdOffset);
  if (reservation == MAP_FAILED) [[unlikely]] {
    goto ret;
  }

  reservation_addr = reinterpret_cast<uintptr_t>(reservation);
  aligned_addr = RoundUp(reservation_addr, alignment);
  prefix_size = aligned_addr - reservation_addr;
  suffix_size = (reservation_addr + size + alignment) - (aligned_addr + size);

  // Linux 允许局部释放 (Partial Unmap)，这是与 Windows 最大的不同。
  // 我们直接将首尾多余的部分归还给 OS，中间留下的就是完美对齐的 size 大小内存。
  if (prefix_size != 0) {
    munmap(reservation, prefix_size);
  }
  if (suffix_size != 0) {
    munmap(reinterpret_cast<void*>(aligned_addr + size), suffix_size);
  }
  address_ = reinterpret_cast<void*>(aligned_addr);

ret:
  if (address_ == MAP_FAILED) [[unlikely]] {
    std::printf("VirtualMemory OOM!\n");
    std::exit(1);
  }
  return;
}

VirtualMemory::~VirtualMemory() {
  if (IsReserved()) {
    if (munmap(address(), size()) == 0) {
      address_ = MAP_FAILED;
    }
  }
}

bool VirtualMemory::IsReserved() {
  return address_ != MAP_FAILED;
}

bool VirtualMemory::Commit(void* address, size_t size, bool is_executable) {
  int prot = PROT_READ | PROT_WRITE | (is_executable ? PROT_EXEC : 0);
  if (MAP_FAILED == mmap(address, size, prot,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, kMmapFd,
                         kMmapFdOffset)) {
    return false;
  }
  return true;
}

bool VirtualMemory::Uncommit(void* address, size_t size) {
  return mmap(address, size, PROT_NONE,
              MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, kMmapFd,
              kMmapFdOffset) != MAP_FAILED;
}
#endif  // BUILDFLAG(IS_LINUX)

}  // namespace saauso::internal
