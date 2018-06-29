// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebScopedVirtualTimePauser_h
#define WebScopedVirtualTimePauser_h

#include "WebCommon.h"
#include "base/time/time.h"

namespace blink {
namespace scheduler {
class RendererSchedulerImpl;
}  // namespace scheduler

// A move only RAII style helper which makes it easier for subsystems to pause
// virtual time while performing an asynchronous operation.
class BLINK_PLATFORM_EXPORT WebScopedVirtualTimePauser {
 public:
  enum class VirtualTaskDuration {
    kInstant,    // Virtual time will not be advanced when it's unpaused.
    kNonInstant  // Virtual time may be advanced when it's unpaused.
  };

  // Note simply creating a WebScopedVirtualTimePauser doesn't cause VirtualTime
  // to pause, instead you need to call PauseVirtualTime.
  WebScopedVirtualTimePauser(scheduler::RendererSchedulerImpl*,
                             VirtualTaskDuration);

  WebScopedVirtualTimePauser();
  ~WebScopedVirtualTimePauser();

  WebScopedVirtualTimePauser(WebScopedVirtualTimePauser&& other);
  WebScopedVirtualTimePauser& operator=(WebScopedVirtualTimePauser&& other);

  WebScopedVirtualTimePauser(const WebScopedVirtualTimePauser&) = delete;
  WebScopedVirtualTimePauser& operator=(const WebScopedVirtualTimePauser&) =
      delete;

  // Virtual time will be paused if any WebScopedVirtualTimePauser votes to
  // pause it, and only unpaused only if all WebScopedVirtualTimePauser are
  // either destroyed or vote to unpause.
  void PauseVirtualTime(bool paused);

 private:
  void DecrementVirtualTimePauseCount();

  base::TimeTicks virtual_time_when_paused_;
  bool paused_ = false;
  VirtualTaskDuration duration_ = VirtualTaskDuration::kInstant;
  scheduler::RendererSchedulerImpl* scheduler_;  // NOT OWNED
  int trace_id_;

  static int next_trace_id_;
};

}  // namespace blink

#endif  // WebScopedVirtualTimePauser_h
