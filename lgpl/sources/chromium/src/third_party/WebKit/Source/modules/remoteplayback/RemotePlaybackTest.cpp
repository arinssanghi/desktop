// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/remoteplayback/RemotePlayback.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "bindings/modules/v8/RemotePlaybackAvailabilityCallback.h"
#include "core/dom/UserGestureIndicator.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/testing/DummyPageHolder.h"
#include "modules/presentation/MockWebPresentationClient.h"
#include "modules/presentation/PresentationController.h"
#include "modules/remoteplayback/HTMLMediaElementRemotePlayback.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/modules/presentation/WebPresentationClient.h"
#include "public/platform/modules/presentation/WebPresentationConnectionCallbacks.h"
#include "public/platform/modules/remoteplayback/WebRemotePlaybackState.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class MockFunction : public ScriptFunction {
 public:
  static ::testing::StrictMock<MockFunction>* Create(
      ScriptState* script_state) {
    return new ::testing::StrictMock<MockFunction>(script_state);
  }

  v8::Local<v8::Function> Bind() { return BindToV8Function(); }

  MOCK_METHOD1(Call, ScriptValue(ScriptValue));

 protected:
  explicit MockFunction(ScriptState* script_state)
      : ScriptFunction(script_state) {}
};

class MockEventListenerForRemotePlayback : public EventListener {
 public:
  MockEventListenerForRemotePlayback() : EventListener(kCPPEventListenerType) {}

  bool operator==(const EventListener& other) const final {
    return this == &other;
  }

  MOCK_METHOD2(handleEvent, void(ExecutionContext* executionContext, Event*));
};

class RemotePlaybackTest : public ::testing::Test {
 protected:
  void SetUp() override {
    was_new_remote_playback_pipeline_enabled_ =
        RuntimeEnabledFeatures::NewRemotePlaybackPipelineEnabled();

    was_remote_playback_backend_enabled_ =
        RuntimeEnabledFeatures::RemotePlaybackBackendEnabled();
    // Pretend the backend is enabled by default to test the API with backend
    // implemented.
    RuntimeEnabledFeatures::SetRemotePlaybackBackendEnabled(true);
  }

  void TearDown() override {
    RuntimeEnabledFeatures::SetNewRemotePlaybackPipelineEnabled(
        was_new_remote_playback_pipeline_enabled_);
    RuntimeEnabledFeatures::SetRemotePlaybackBackendEnabled(
        was_remote_playback_backend_enabled_);
  }

  void CancelPrompt(RemotePlayback* remote_playback) {
    remote_playback->PromptCancelled();
  }

  void SetState(RemotePlayback* remote_playback, WebRemotePlaybackState state) {
    remote_playback->StateChanged(state);
  }

  bool IsListening(RemotePlayback* remote_playback) {
    return remote_playback->is_listening_;
  }

  // Has to outlive the page so that PresentationController doesn't crash trying
  // to set it to null in ContextDestroyed().
  MockWebPresentationClient presentation_client_;

 private:
  bool was_remote_playback_backend_enabled_;
  bool was_new_remote_playback_pipeline_enabled_;
};

TEST_F(RemotePlaybackTest, PromptCancelledRejectsWithNotAllowedError) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  auto resolve = MockFunction::Create(scope.GetScriptState());
  auto reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(0);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(1);

  std::unique_ptr<UserGestureIndicator> indicator =
      LocalFrame::CreateUserGesture(&page_holder->GetFrame(),
                                    UserGestureToken::kNewGesture);
  remote_playback->prompt(scope.GetScriptState())
      .Then(resolve->Bind(), reject->Bind());
  CancelPrompt(remote_playback);

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
}

TEST_F(RemotePlaybackTest, PromptConnectedRejectsWhenCancelled) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  auto resolve = MockFunction::Create(scope.GetScriptState());
  auto reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(0);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(1);

  SetState(remote_playback, WebRemotePlaybackState::kConnected);

  std::unique_ptr<UserGestureIndicator> indicator =
      LocalFrame::CreateUserGesture(&page_holder->GetFrame(),
                                    UserGestureToken::kNewGesture);
  remote_playback->prompt(scope.GetScriptState())
      .Then(resolve->Bind(), reject->Bind());
  CancelPrompt(remote_playback);

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
}

TEST_F(RemotePlaybackTest, PromptConnectedResolvesWhenDisconnected) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  auto resolve = MockFunction::Create(scope.GetScriptState());
  auto reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(1);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(0);

  SetState(remote_playback, WebRemotePlaybackState::kConnected);

  std::unique_ptr<UserGestureIndicator> indicator =
      LocalFrame::CreateUserGesture(&page_holder->GetFrame(),
                                    UserGestureToken::kNewGesture);
  remote_playback->prompt(scope.GetScriptState())
      .Then(resolve->Bind(), reject->Bind());

  SetState(remote_playback, WebRemotePlaybackState::kDisconnected);

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
}

TEST_F(RemotePlaybackTest, StateChangeEvents) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  auto connecting_handler =
      new ::testing::StrictMock<MockEventListenerForRemotePlayback>();
  auto connect_handler =
      new ::testing::StrictMock<MockEventListenerForRemotePlayback>();
  auto disconnect_handler =
      new ::testing::StrictMock<MockEventListenerForRemotePlayback>();

  remote_playback->addEventListener(EventTypeNames::connecting,
                                    connecting_handler);
  remote_playback->addEventListener(EventTypeNames::connect, connect_handler);
  remote_playback->addEventListener(EventTypeNames::disconnect,
                                    disconnect_handler);

  EXPECT_CALL(*connecting_handler, handleEvent(::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*connect_handler, handleEvent(::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*disconnect_handler, handleEvent(::testing::_, ::testing::_))
      .Times(1);

  SetState(remote_playback, WebRemotePlaybackState::kConnecting);
  SetState(remote_playback, WebRemotePlaybackState::kConnecting);
  SetState(remote_playback, WebRemotePlaybackState::kConnected);
  SetState(remote_playback, WebRemotePlaybackState::kConnected);
  SetState(remote_playback, WebRemotePlaybackState::kDisconnected);
  SetState(remote_playback, WebRemotePlaybackState::kDisconnected);

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(connecting_handler);
  ::testing::Mock::VerifyAndClear(connect_handler);
  ::testing::Mock::VerifyAndClear(disconnect_handler);
}

TEST_F(RemotePlaybackTest,
       DisableRemotePlaybackRejectsPromptWithInvalidStateError) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  MockFunction* resolve = MockFunction::Create(scope.GetScriptState());
  MockFunction* reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(0);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(1);

  std::unique_ptr<UserGestureIndicator> indicator =
      LocalFrame::CreateUserGesture(&page_holder->GetFrame(),
                                    UserGestureToken::kNewGesture);
  remote_playback->prompt(scope.GetScriptState())
      .Then(resolve->Bind(), reject->Bind());
  HTMLMediaElementRemotePlayback::SetBooleanAttribute(
      HTMLNames::disableremoteplaybackAttr, *element, true);

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
}

TEST_F(RemotePlaybackTest, DisableRemotePlaybackCancelsAvailabilityCallbacks) {
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  MockFunction* callback_function =
      MockFunction::Create(scope.GetScriptState());
  RemotePlaybackAvailabilityCallback* availability_callback =
      RemotePlaybackAvailabilityCallback::Create(scope.GetScriptState(),
                                                 callback_function->Bind());

  // The initial call upon registering will not happen as it's posted on the
  // message loop.
  EXPECT_CALL(*callback_function, Call(::testing::_)).Times(0);

  MockFunction* resolve = MockFunction::Create(scope.GetScriptState());
  MockFunction* reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(1);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(0);

  remote_playback
      ->watchAvailability(scope.GetScriptState(), availability_callback)
      .Then(resolve->Bind(), reject->Bind());

  HTMLMediaElementRemotePlayback::SetBooleanAttribute(
      HTMLNames::disableremoteplaybackAttr, *element, true);

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
  ::testing::Mock::VerifyAndClear(callback_function);
}

TEST_F(RemotePlaybackTest, PromptThrowsWhenBackendDisabled) {
  RuntimeEnabledFeatures::SetRemotePlaybackBackendEnabled(false);
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  auto resolve = MockFunction::Create(scope.GetScriptState());
  auto reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(0);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(1);

  std::unique_ptr<UserGestureIndicator> indicator =
      LocalFrame::CreateUserGesture(&page_holder->GetFrame(),
                                    UserGestureToken::kNewGesture);
  remote_playback->prompt(scope.GetScriptState())
      .Then(resolve->Bind(), reject->Bind());

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
}

TEST_F(RemotePlaybackTest, WatchAvailabilityWorksWhenBackendDisabled) {
  RuntimeEnabledFeatures::SetRemotePlaybackBackendEnabled(false);
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  MockFunction* callback_function =
      MockFunction::Create(scope.GetScriptState());
  RemotePlaybackAvailabilityCallback* availability_callback =
      RemotePlaybackAvailabilityCallback::Create(scope.GetScriptState(),
                                                 callback_function->Bind());

  // The initial call upon registering will not happen as it's posted on the
  // message loop.
  EXPECT_CALL(*callback_function, Call(::testing::_)).Times(0);

  MockFunction* resolve = MockFunction::Create(scope.GetScriptState());
  MockFunction* reject = MockFunction::Create(scope.GetScriptState());

  EXPECT_CALL(*resolve, Call(::testing::_)).Times(1);
  EXPECT_CALL(*reject, Call(::testing::_)).Times(0);

  remote_playback
      ->watchAvailability(scope.GetScriptState(), availability_callback)
      .Then(resolve->Bind(), reject->Bind());

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(resolve);
  ::testing::Mock::VerifyAndClear(reject);
  ::testing::Mock::VerifyAndClear(callback_function);
}

TEST_F(RemotePlaybackTest, IsListening) {
  RuntimeEnabledFeatures::SetNewRemotePlaybackPipelineEnabled(true);
  V8TestingScope scope;

  auto page_holder = DummyPageHolder::Create();

  HTMLMediaElement* element =
      HTMLVideoElement::Create(page_holder->GetDocument());
  RemotePlayback* remote_playback =
      HTMLMediaElementRemotePlayback::remote(*element);

  PresentationController::ProvideTo(page_holder->GetFrame(),
                                    &presentation_client_);

  EXPECT_CALL(presentation_client_,
              StartListening(::testing::Eq(remote_playback)))
      .Times(2);
  EXPECT_CALL(presentation_client_,
              StopListening(::testing::Eq(remote_playback)))
      .Times(2);

  MockFunction* callback_function =
      MockFunction::Create(scope.GetScriptState());
  RemotePlaybackAvailabilityCallback* availability_callback =
      RemotePlaybackAvailabilityCallback::Create(scope.GetScriptState(),
                                                 callback_function->Bind());

  // The initial call upon registering will not happen as it's posted on the
  // message loop.
  EXPECT_CALL(*callback_function, Call(::testing::_)).Times(2);

  remote_playback->watchAvailability(scope.GetScriptState(),
                                     availability_callback);

  ASSERT_TRUE(remote_playback->Urls().empty());
  ASSERT_FALSE(IsListening(remote_playback));

  remote_playback->SourceChanged(
      WebURL(KURL(kParsedURLString, "http://www.example.com")), true);
  ASSERT_EQ((size_t)1, remote_playback->Urls().size());
  ASSERT_TRUE(IsListening(remote_playback));
  remote_playback->AvailabilityChanged(mojom::ScreenAvailability::AVAILABLE);

  remote_playback->cancelWatchAvailability(scope.GetScriptState());
  ASSERT_EQ((size_t)1, remote_playback->Urls().size());
  ASSERT_FALSE(IsListening(remote_playback));

  remote_playback->watchAvailability(scope.GetScriptState(),
                                     availability_callback);
  ASSERT_EQ((size_t)1, remote_playback->Urls().size());
  ASSERT_TRUE(IsListening(remote_playback));
  remote_playback->AvailabilityChanged(mojom::ScreenAvailability::AVAILABLE);

  remote_playback->SourceChanged(WebURL(), false);
  ASSERT_TRUE(remote_playback->Urls().empty());
  ASSERT_FALSE(IsListening(remote_playback));

  remote_playback->SourceChanged(WebURL(KURL(kParsedURLString, "@$@#@#")),
                                 true);
  ASSERT_TRUE(remote_playback->Urls().empty());
  ASSERT_FALSE(IsListening(remote_playback));

  // Runs pending promises.
  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());

  // Verify mock expectations explicitly as the mock objects are garbage
  // collected.
  ::testing::Mock::VerifyAndClear(callback_function);
  ::testing::Mock::VerifyAndClear(&presentation_client_);
}

}  // namespace blink
