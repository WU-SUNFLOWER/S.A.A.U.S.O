// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cassert>

#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/utils/allocation.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX)
#include <sys/mman.h>
#include <sys/types.h>
#endif  // BUILDFLAG(IS_WIN)

namespace saauso::internal {

void* VirtualMemory::address() {
  assert(IsReserved());
  return address_;
}

#if BUILDFLAG(IS_WIN)
VirtualMemory::VirtualMemory(size_t size) {
  address_ = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
  size_ = size;
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
  address_ =
      mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
           kMmapFd, kMmapFdOffset);
  size_ = size;
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
