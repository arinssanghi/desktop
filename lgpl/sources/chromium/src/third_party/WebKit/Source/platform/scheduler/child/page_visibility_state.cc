// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/page_visibility_state.h"

namespace blink {
namespace scheduler {

const char* PageVisibilityStateToString(PageVisibilityState visibility) {
  switch (visibility) {
    case PageVisibilityState::kVisible:
      return "visible";
    case PageVisibilityState::kHidden:
      return "hidden";
  }
  // Keep MSVC happy.
  return nullptr;
}

}  // namespace scheduler
}  // namespace blink
