/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Performance_h
#define Performance_h

#include "core/CoreExport.h"
#include "core/dom/DOMHighResTimeStamp.h"
#include "core/dom/events/EventTarget.h"
#include "core/loader/FrameLoaderTypes.h"
#include "core/timing/PerformanceEntry.h"
#include "core/timing/PerformanceNavigationTiming.h"
#include "core/timing/PerformancePaintTiming.h"
#include "core/timing/SubTaskAttribution.h"
#include "platform/Timer.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/LinkedHashSet.h"
#include "platform/wtf/Vector.h"
#include "public/platform/WebResourceTimingInfo.h"

namespace blink {

class DoubleOrPerformanceMarkOptions;
class ExceptionState;
class PerformanceObserver;
class PerformanceTiming;
class ResourceResponse;
class ResourceTimingInfo;
class SecurityOrigin;
class UserTiming;
class ScriptState;
class ScriptValue;
class SubTaskAttribution;
class V8ObjectBuilder;

using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;

class CORE_EXPORT Performance : public EventTargetWithInlineData {
 public:
  ~Performance() override;

  const AtomicString& InterfaceName() const override;

  virtual PerformanceTiming* timing() const;

  virtual void UpdateLongTaskInstrumentation() {}

  // Reduce the resolution to prevent timing attacks. See:
  // http://www.w3.org/TR/hr-time-2/#privacy-security
  static double ClampTimeResolution(double time_seconds);

  static DOMHighResTimeStamp MonotonicTimeToDOMHighResTimeStamp(
      TimeTicks time_origin,
      TimeTicks monotonic_time,
      bool allow_negative_value);

  // Translate given platform monotonic time in seconds into a high resolution
  // DOMHighResTimeStamp in milliseconds. The result timestamp is relative to
  // document's time origin and has a time resolution that is safe for
  // exposing to web.
  DOMHighResTimeStamp MonotonicTimeToDOMHighResTimeStamp(TimeTicks) const;
  DOMHighResTimeStamp now() const;

  // High Resolution Time Level 3 timeOrigin.
  // (https://www.w3.org/TR/hr-time-3/#dom-performance-timeorigin)
  DOMHighResTimeStamp timeOrigin() const;

  // Internal getter method for the time origin value.
  double GetTimeOrigin() const { return TimeTicksInSeconds(time_origin_); }

  PerformanceEntryVector getEntries();
  PerformanceEntryVector getEntriesByType(const String& entry_type);
  PerformanceEntryVector getEntriesByName(const String& name,
                                          const String& entry_type);

  void clearResourceTimings();
  void setResourceTimingBufferSize(unsigned);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(resourcetimingbufferfull);

  void AddLongTaskTiming(
      TimeTicks start_time,
      TimeTicks end_time,
      const String& name,
      const String& culprit_frame_src,
      const String& culprit_frame_id,
      const String& culprit_frame_name,
      const SubTaskAttribution::EntriesVector& sub_task_attributions);

  // Generates and add a performance entry for the given ResourceTimingInfo.
  // |overridden_initiator_type| allows the initiator type to be overridden to
  // the frame element name for the main resource.
  void GenerateAndAddResourceTiming(
      const ResourceTimingInfo&,
      const AtomicString& overridden_initiator_type = g_null_atom);
  // Generates timing info suitable for appending to the performance entries of
  // a context with |origin|. This should be rarely used; most callsites should
  // prefer the convenience method |GenerateAndAddResourceTiming()|.
  static WebResourceTimingInfo GenerateResourceTiming(
      const SecurityOrigin& destination_origin,
      const ResourceTimingInfo&,
      ExecutionContext& context_for_use_counter);
  void AddResourceTiming(const WebResourceTimingInfo&,
                         const AtomicString& initiator_type = g_null_atom);

  void NotifyNavigationTimingToObservers();

  void AddFirstPaintTiming(TimeTicks start_time);

  void AddFirstContentfulPaintTiming(TimeTicks start_time);

  void mark(ScriptState*, const String& mark_name, ExceptionState&);

  void mark(ScriptState*,
            const String& mark_name,
            DoubleOrPerformanceMarkOptions& start_time_or_mark_options,
            ExceptionState&);

  void clearMarks(const String& mark_name);

  void measure(const String& measure_name,
               const String& start_mark,
               const String& end_mark,
               ExceptionState&);
  void clearMeasures(const String& measure_name);

  void UnregisterPerformanceObserver(PerformanceObserver&);
  void RegisterPerformanceObserver(PerformanceObserver&);
  void UpdatePerformanceObserverFilterOptions();
  void ActivateObserver(PerformanceObserver&);
  void ResumeSuspendedObservers();

  // This enum is used to index different possible strings for for UMA enum
  // histogram. New enum values can be added, but existing enums must never be
  // renumbered or deleted and reused.
  // This enum should be consistent with PerformanceMeasurePassedInParameterType
  // in tools/metrics/histograms/enums.xml.
  enum PerformanceMeasurePassedInParameterType {
    kObjectObject = 0,
    // 1 to 8 are navigation-timing types.
    kUnloadEventStart = 1,
    kUnloadEventEnd = 2,
    kDomInteractive = 3,
    kDomContentLoadedEventStart = 4,
    kDomContentLoadedEventEnd = 5,
    kDomComplete = 6,
    kLoadEventStart = 7,
    kLoadEventEnd = 8,
    kOther = 9,
    kPerformanceMeasurePassedInParameterCount
  };

  static PerformanceMeasurePassedInParameterType
  ToPerformanceMeasurePassedInParameterType(const String& s) {
    // All passed-in objects will be stringified into this type.
    if (s == "[object Object]")
      return kObjectObject;
    // The following names come from
    // https://w3c.github.io/navigation-timing/#sec-PerformanceNavigationTiming.
    if (s == "unloadEventStart")
      return kUnloadEventStart;
    if (s == "unloadEventEnd")
      return kUnloadEventEnd;
    if (s == "domInteractive")
      return kDomInteractive;
    if (s == "domContentLoadedEventStart")
      return kDomContentLoadedEventStart;
    if (s == "domContentLoadedEventEnd")
      return kDomContentLoadedEventEnd;
    if (s == "domComplete")
      return kDomComplete;
    if (s == "loadEventStart")
      return kLoadEventStart;
    if (s == "loadEventEnd")
      return kLoadEventEnd;
    return kOther;
  }

  static bool AllowsTimingRedirect(const Vector<ResourceResponse>&,
                                   const ResourceResponse&,
                                   const SecurityOrigin&,
                                   ExecutionContext*);

  ScriptValue toJSONForBinding(ScriptState*) const;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(const ScriptWrappableVisitor*) const override;

 private:
  static bool PassesTimingAllowCheck(const ResourceResponse&,
                                     const SecurityOrigin&,
                                     const AtomicString&,
                                     ExecutionContext*);

  void AddPaintTiming(PerformancePaintTiming::PaintType, TimeTicks start_time);

 protected:
  Performance(TimeTicks time_origin,
              scoped_refptr<base::SingleThreadTaskRunner>);

  // Expect Performance to override this method,
  // WorkerPerformance doesn't have to override this.
  virtual PerformanceNavigationTiming* CreateNavigationTimingInstance() {
    return nullptr;
  }

  bool IsResourceTimingBufferFull();
  void AddResourceTimingBuffer(PerformanceEntry&);

  void NotifyObserversOfEntry(PerformanceEntry&) const;
  void NotifyObserversOfEntries(PerformanceEntryVector&);
  bool HasObserverFor(PerformanceEntry::EntryType) const;

  void DeliverObservationsTimerFired(TimerBase*);

  virtual void BuildJSONValue(V8ObjectBuilder&) const;

  PerformanceEntryVector frame_timing_buffer_;
  unsigned frame_timing_buffer_size_;
  PerformanceEntryVector resource_timing_buffer_;
  unsigned resource_timing_buffer_size_;
  Member<PerformanceEntry> navigation_timing_;
  Member<UserTiming> user_timing_;
  Member<PerformanceEntry> first_paint_timing_;
  Member<PerformanceEntry> first_contentful_paint_timing_;

  TimeTicks time_origin_;

  PerformanceEntryTypeMask observer_filter_options_;
  HeapLinkedHashSet<TraceWrapperMember<PerformanceObserver>> observers_;
  HeapLinkedHashSet<Member<PerformanceObserver>> active_observers_;
  HeapLinkedHashSet<Member<PerformanceObserver>> suspended_observers_;
  TaskRunnerTimer<Performance> deliver_observations_timer_;
};

}  // namespace blink

#endif  // Performance_h
