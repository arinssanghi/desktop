// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletProcessor_h
#define AudioWorkletProcessor_h

#include "modules/ModulesExport.h"
#include "modules/webaudio/AudioWorkletProcessorErrorState.h"
#include "platform/audio/AudioArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"
#include "v8/include/v8.h"

namespace blink {

class AudioBus;
class AudioWorkletGlobalScope;
class AudioWorkletProcessorDefinition;
class MessagePort;
class ExecutionContext;

// AudioWorkletProcessor class represents the active instance created from
// AudioWorkletProcessorDefinition. |AudioWorkletNodeHandler| invokes
// process() method in this object upon graph rendering.
//
// This is constructed and destroyed on a worker thread, and all methods also
// must be called on the worker thread.
class MODULES_EXPORT AudioWorkletProcessor : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // This static factory should be called after an instance of
  // |AudioWorkletNode| gets created by user-supplied JS code in the main
  // thread. This factory must not be called by user in
  // |AudioWorkletGlobalScope|.
  static AudioWorkletProcessor* Create(ExecutionContext*);

  ~AudioWorkletProcessor() = default;

  // |AudioWorkletHandler| invokes this method to process audio.
  bool Process(
      Vector<AudioBus*>* input_buses,
      Vector<AudioBus*>* output_buses,
      HashMap<String, std::unique_ptr<AudioFloatArray>>* param_value_map);

  const String& Name() const { return name_; }

  void SetErrorState(AudioWorkletProcessorErrorState);
  AudioWorkletProcessorErrorState GetErrorState() const;
  bool hasErrorOccured() const;

  // IDL
  MessagePort* port() const;

  void Trace(blink::Visitor*);

 private:
  AudioWorkletProcessor(AudioWorkletGlobalScope*,
                        const String& name,
                        MessagePort*);

  Member<AudioWorkletGlobalScope> global_scope_;
  Member<MessagePort> processor_port_;

  const String name_;

  AudioWorkletProcessorErrorState error_state_ =
      AudioWorkletProcessorErrorState::kNoError;
};

}  // namespace blink

#endif  // AudioWorkletProcessor_h
