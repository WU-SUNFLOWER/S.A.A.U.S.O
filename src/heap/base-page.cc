// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/base-page.h"

#include <cassert>

namespace saauso::internal {

namespace {

void CheckValidAddressInPageOnlyForDebug(BasePage* page, Address addr) {
  assert(page->magic() == BasePage::kMagicHeader);
  assert(page->area_start() <= addr && addr < page->allocation_top());
}

}  // namespace

bool BasePage::HasFlag(uintptr_t flag) const {
  return (flags_ & flag) != 0;
}

void BasePage::SetFlag(uintptr_t flag) {
  flags_ |= flag;
}

void BasePage::ClearFlag(uintptr_t flag) {
  flags_ &= ~flag;
}

// static
Address BasePage::PageBase(Address addr) {
  return addr & ~(static_cast<Address>(kPageSizeInBytes) - 1);
}

// static
BasePage* BasePage::FromAddress(Address addr) {
  return reinterpret_cast<BasePage*>(PageBase(addr));
}

// static
bool BasePage::InEdenPage(Address addr_in_page) {
  BasePage* page = FromAddress(addr_in_page);
  CheckValidAddressInPageOnlyForDebug(page, addr_in_page);
  return page->HasFlag(Flag::kEdenPage);
}

// static
bool BasePage::InSurvivorPage(Address addr_in_page) {
  BasePage* page = FromAddress(addr_in_page);
  CheckValidAddressInPageOnlyForDebug(page, addr_in_page);
  return page->HasFlag(Flag::kSurvivorPage);
}

// static
bool BasePage::InYoungPage(Address addr_in_page) {
  BasePage* page = FromAddress(addr_in_page);
  CheckValidAddressInPageOnlyForDebug(page, addr_in_page);
  return page->HasFlag(Flag::kSurvivorPage | Flag::kEdenPage);
}

// static
bool BasePage::InOldPage(Address addr_in_page) {
  BasePage* page = FromAddress(addr_in_page);
  CheckValidAddressInPageOnlyForDebug(page, addr_in_page);
  return page->HasFlag(Flag::kOldPage);
}

// static
bool BasePage::InMetaPage(Address addr_in_page) {
  BasePage* page = FromAddress(addr_in_page);
  CheckValidAddressInPageOnlyForDebug(page, addr_in_page);
  return page->HasFlag(Flag::kMetaPage);
}

}  // namespace saauso::internal
