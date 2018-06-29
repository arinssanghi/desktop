/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef Notification_h
#define Notification_h

#include "bindings/core/v8/ActiveScriptWrappable.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/DOMTimeStamp.h"
#include "modules/EventTargetModules.h"
#include "modules/ModulesExport.h"
#include "modules/vibration/NavigatorVibration.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/AsyncMethodRunner.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "public/platform/modules/notifications/WebNotificationData.h"
#include "public/platform/modules/notifications/notification_service.mojom-blink.h"
#include "public/platform/modules/permissions/permission.mojom-blink.h"
#include "public/platform/modules/permissions/permission_status.mojom-blink.h"

namespace blink {

class ExecutionContext;
class NotificationOptions;
class NotificationResourcesLoader;
class ScriptState;
class V8NotificationPermissionCallback;

class MODULES_EXPORT Notification final
    : public EventTargetWithInlineData,
      public ActiveScriptWrappable<Notification>,
      public ContextLifecycleObserver,
      public mojom::blink::NonPersistentNotificationListener {
  USING_GARBAGE_COLLECTED_MIXIN(Notification);
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Used for JavaScript instantiations of non-persistent notifications. Will
  // automatically schedule for the notification to be displayed to the user
  // when the developer-provided data is valid.
  static Notification* Create(ExecutionContext*,
                              const String& title,
                              const NotificationOptions&,
                              ExceptionState&);

  // Used for embedder-created persistent notifications. Initializes the state
  // of the notification as either Showing or Closed based on |showing|.
  static Notification* Create(ExecutionContext*,
                              const String& notification_id,
                              const WebNotificationData&,
                              bool showing);

  ~Notification() override;

  void close();

  DEFINE_ATTRIBUTE_EVENT_LISTENER(click);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(show);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(close);

  // NonPersistentNotificationListener interface.
  void OnShow() override;
  void OnClick() override;
  void OnClose() override;

  String title() const;
  String dir() const;
  String lang() const;
  String body() const;
  String tag() const;
  String image() const;
  String icon() const;
  String badge() const;
  NavigatorVibration::VibrationPattern vibrate() const;
  DOMTimeStamp timestamp() const;
  bool renotify() const;
  bool silent() const;
  bool requireInteraction() const;
  ScriptValue data(ScriptState*);
  Vector<v8::Local<v8::Value>> actions(ScriptState*) const;

  static String PermissionString(mojom::blink::PermissionStatus);
  static String permission(ExecutionContext*);
  static ScriptPromise requestPermission(
      ScriptState*,
      V8NotificationPermissionCallback* deprecated_callback = nullptr);

  static size_t maxActions();

  // EventTarget interface.
  ExecutionContext* GetExecutionContext() const final {
    return ContextLifecycleObserver::GetExecutionContext();
  }
  const AtomicString& InterfaceName() const override;

  // ContextLifecycleObserver interface.
  void ContextDestroyed(ExecutionContext*) override;

  // ScriptWrappable interface.
  bool HasPendingActivity() const final;

  virtual void Trace(blink::Visitor*);

 protected:
  // EventTarget interface.
  DispatchEventResult DispatchEventInternal(Event*) final;

 private:
  // The type of notification this instance represents. Non-persistent
  // notifications will have events delivered to their instance, whereas
  // persistent notification will be using a Service Worker.
  enum class Type { kNonPersistent, kPersistent };

  // The current phase of the notification in its lifecycle.
  enum class State { kLoading, kShowing, kClosing, kClosed };

  Notification(ExecutionContext*, Type, const WebNotificationData&);

  // Sets the state of the notification in its lifecycle.
  void SetState(State state) { state_ = state; }

  // Sets the notification ID to |notificationId|. This should be done once
  // the notification has shown for non-persistent notifications, and at
  // object initialisation time for persistent notifications.
  void SetNotificationId(const String& notification_id) {
    notification_id_ = notification_id;
  }

  // Sets the token which will be used to both show and close the notification.
  // Should be equal to tag_ if a tag is present, else should be unique.
  void SetToken(const String& token) { token_ = token; }

  // Schedules an asynchronous call to |prepareShow|, allowing the constructor
  // to return so that events can be fired on the notification object.
  void SchedulePrepareShow();

  // Verifies that permission has been granted, then asynchronously starts
  // loading the resources associated with this notification.
  void PrepareShow();

  // Shows the notification through the embedder using the loaded resources.
  void DidLoadResources(NotificationResourcesLoader*);

  void DispatchErrorEvent();

  Type type_;
  State state_;

  WebNotificationData data_;

  String notification_id_;

  String token_;

  Member<AsyncMethodRunner<Notification>> prepare_show_method_runner_;

  Member<NotificationResourcesLoader> loader_;

  mojo::Binding<mojom::blink::NonPersistentNotificationListener>
      listener_binding_;
};

}  // namespace blink

#endif  // Notification_h
