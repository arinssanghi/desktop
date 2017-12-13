/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ApplicationCache_h
#define ApplicationCache_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/events/EventTarget.h"
#include "core/loader/appcache/ApplicationCacheHost.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class ExceptionState;
class LocalFrame;

class ApplicationCache final : public EventTargetWithInlineData,
                               public DOMWindowClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(ApplicationCache);

 public:
  static ApplicationCache* Create(LocalFrame* frame) {
    return new ApplicationCache(frame);
  }
  ~ApplicationCache() override {}

  unsigned short status() const;
  void update(ExceptionState&);
  void swapCache(ExceptionState&);
  void abort();

  // Explicitly named attribute event listener helpers

  DEFINE_ATTRIBUTE_EVENT_LISTENER(checking);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(noupdate);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(downloading);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(updateready);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(cached);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(obsolete);

  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  static const AtomicString& ToEventType(ApplicationCacheHost::EventID);

  DECLARE_VIRTUAL_TRACE();

 private:
  explicit ApplicationCache(LocalFrame*);

  void RecordAPIUseType() const;

  ApplicationCacheHost* GetApplicationCacheHost() const;
};

}  // namespace blink

#endif  // ApplicationCache_h
