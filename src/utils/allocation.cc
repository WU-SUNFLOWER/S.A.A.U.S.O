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
}  // namespace

void* VirtualMemory::address() {
  assert(IsReserved());
  return address_;
}

#if BUILDFLAG(IS_WIN)
VirtualMemory::VirtualMemory(size_t size) { Reserve(size, 0); }

VirtualMemory::VirtualMemory(size_t size, size_t alignment) {
  Reserve(size, alignment);
}

void VirtualMemory::Reserve(size_t size, size_t alignment) {
  size_ = size;
  address_ = nullptr;

  if (alignment == 0) {
    address_ = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    return;
  }

  assert((alignment & (alignment - 1)) == 0);
  constexpr int kMaxAttempts = 64;
  for (int i = 0; i < kMaxAttempts; ++i) {
    void* probe = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    if (probe == nullptr) {
      return;
    }
    if ((reinterpret_cast<uintptr_t>(probe) & (alignment - 1)) == 0) {
      address_ = probe;
      return;
    }

    uintptr_t probe_addr = reinterpret_cast<uintptr_t>(probe);
    VirtualFree(probe, 0, MEM_RELEASE);

    uintptr_t aligned_up = (probe_addr + alignment - 1) & ~(alignment - 1);
    void* aligned = VirtualAlloc(reinterpret_cast<void*>(aligned_up), size,
                                 MEM_RESERVE, PAGE_NOACCESS);
    if (aligned != nullptr) {
      address_ = aligned;
      return;
    }

    uintptr_t aligned_down = probe_addr & ~(alignment - 1);
    if (aligned_down != 0 && aligned_down != aligned_up) {
      aligned = VirtualAlloc(reinterpret_cast<void*>(aligned_down), size,
                             MEM_RESERVE, PAGE_NOACCESS);
      if (aligned != nullptr) {
        address_ = aligned;
        return;
      }
    }
  }
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
VirtualMemory::VirtualMemory(size_t size) { Reserve(size, 0); }

VirtualMemory::VirtualMemory(size_t size, size_t alignment) {
  Reserve(size, alignment);
}

void VirtualMemory::Reserve(size_t size, size_t alignment) {
  size_ = size;
  if (alignment == 0) {
    address_ =
        mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
             kMmapFd, kMmapFdOffset);
    return;
  }

  assert((alignment & (alignment - 1)) == 0);
  void* reservation = mmap(nullptr, size + alignment, PROT_NONE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                           kMmapFd, kMmapFdOffset);
  if (reservation == MAP_FAILED) {
    address_ = MAP_FAILED;
    return;
  }

  uintptr_t reservation_addr = reinterpret_cast<uintptr_t>(reservation);
  uintptr_t aligned_addr = (reservation_addr + alignment - 1) & ~(alignment - 1);
  size_t prefix_size = aligned_addr - reservation_addr;
  size_t suffix_size =
      (reservation_addr + size + alignment) - (aligned_addr + size);

  if (prefix_size != 0) {
    munmap(reservation, prefix_size);
  }
  if (suffix_size != 0) {
    munmap(reinterpret_cast<void*>(aligned_addr + size), suffix_size);
  }
  address_ = reinterpret_cast<void*>(aligned_addr);
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
