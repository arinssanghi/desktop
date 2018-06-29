// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InspectorPerformanceAgent_h
#define InspectorPerformanceAgent_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "core/inspector/InspectorBaseAgent.h"
#include "core/inspector/protocol/Performance.h"
#include "platform/scheduler/base/task_time_observer.h"
#include "platform/wtf/Time.h"

namespace blink {

class InspectedFrames;

namespace probe {
class CallFunction;
class ExecuteScript;
class RecalculateStyle;
class UpdateLayout;
}  // namespace probe

class CORE_EXPORT InspectorPerformanceAgent final
    : public InspectorBaseAgent<protocol::Performance::Metainfo>,
      public scheduler::TaskTimeObserver {
 public:
  void Trace(blink::Visitor*) override;

  static InspectorPerformanceAgent* Create(InspectedFrames* inspected_frames) {
    return new InspectorPerformanceAgent(inspected_frames);
  }
  ~InspectorPerformanceAgent() override;

  void Restore() override;

  // Performance protocol domain implementation.
  protocol::Response enable() override;
  protocol::Response disable() override;
  protocol::Response getMetrics(
      std::unique_ptr<protocol::Array<protocol::Performance::Metric>>*
          out_result) override;

  // PerformanceMetrics probes implementation.
  void ConsoleTimeStamp(const String& title);
  void Will(const probe::CallFunction&);
  void Did(const probe::CallFunction&);
  void Will(const probe::ExecuteScript&);
  void Did(const probe::ExecuteScript&);
  void Will(const probe::RecalculateStyle&);
  void Did(const probe::RecalculateStyle&);
  void Will(const probe::UpdateLayout&);
  void Did(const probe::UpdateLayout&);

  // scheduler::TaskTimeObserver
  void WillProcessTask(double start_time) override;
  void DidProcessTask(double start_time, double end_time) override;

 private:
  InspectorPerformanceAgent(InspectedFrames*);

  Member<InspectedFrames> inspected_frames_;
  bool enabled_ = false;
  TimeDelta layout_duration_;
  TimeDelta recalc_style_duration_;
  TimeDelta script_duration_;
  TimeTicks script_start_time_;
  TimeDelta task_duration_;
  TimeTicks task_start_time_;
  unsigned long long layout_count_ = 0;
  unsigned long long recalc_style_count_ = 0;
  int script_call_depth_ = 0;
  int layout_depth_ = 0;

  DISALLOW_COPY_AND_ASSIGN(InspectorPerformanceAgent);
};

}  // namespace blink

#endif  // !defined(InspectorPerformanceAgent_h)
