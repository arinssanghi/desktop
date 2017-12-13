// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/css/CSSTimingData.h"

#include "core/animation/Timing.h"

namespace blink {

CSSTimingData::CSSTimingData() {
  delay_list_.push_back(InitialDelay());
  duration_list_.push_back(InitialDuration());
  timing_function_list_.push_back(InitialTimingFunction());
}

CSSTimingData::CSSTimingData(const CSSTimingData& other)
    : delay_list_(other.delay_list_),
      duration_list_(other.duration_list_),
      timing_function_list_(other.timing_function_list_) {}

Timing CSSTimingData::ConvertToTiming(size_t index) const {
  Timing timing;
  timing.start_delay = GetRepeated(delay_list_, index);
  timing.iteration_duration = GetRepeated(duration_list_, index);
  timing.timing_function = GetRepeated(timing_function_list_, index);
  timing.AssertValid();
  return timing;
}

bool CSSTimingData::TimingMatchForStyleRecalc(
    const CSSTimingData& other) const {
  if (delay_list_ != other.delay_list_)
    return false;
  if (duration_list_ != other.duration_list_)
    return false;
  if (timing_function_list_.size() != other.timing_function_list_.size())
    return false;

  for (size_t i = 0; i < timing_function_list_.size(); i++) {
    if (!DataEquivalent(timing_function_list_.at(i),
                        other.timing_function_list_.at(i))) {
      return false;
    }
  }
  return true;
}

}  // namespace blink
