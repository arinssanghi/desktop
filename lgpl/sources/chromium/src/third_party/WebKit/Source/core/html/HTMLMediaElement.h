/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLMediaElement_h
#define HTMLMediaElement_h

#include <memory>
#include "bindings/core/v8/Nullable.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/CoreExport.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/SuspendableObject.h"
#include "core/dom/events/MediaElementEventQueue.h"
#include "core/html/HTMLElement.h"
#include "core/html/media/MediaControls.h"
#include "core/html/track/TextTrack.h"
#include "platform/Supplementable.h"
#include "platform/WebTaskRunner.h"
#include "platform/audio/AudioSourceProvider.h"
#include "platform/bindings/ActiveScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/network/mime/MIMETypeRegistry.h"
#include "public/platform/WebAudioSourceProviderClient.h"
#include "public/platform/WebMediaPlayerClient.h"
#include "public/platform/WebString.h"

namespace blink {

class AudioSourceProviderClient;
class AudioTrack;
class AudioTrackList;
class AutoplayPolicy;
class ContentType;
class CueTimeline;
class EnumerationHistogram;
class Event;
class ExceptionState;
class HTMLMediaElementControlsList;
class HTMLSourceElement;
class HTMLTrackElement;
class KURL;
class MediaError;
class MediaStreamDescriptor;
class HTMLMediaSource;
class ScriptState;
class TextTrackContainer;
class TextTrackList;
class TimeRanges;
class URLRegistry;
class VideoTrack;
class VideoTrackList;
class WebAudioSourceProvider;
class WebInbandTextTrack;
class WebLayer;
class WebRemotePlaybackClient;

class CORE_EXPORT HTMLMediaElement
    : public HTMLElement,
      public Supplementable<HTMLMediaElement>,
      public ActiveScriptWrappable<HTMLMediaElement>,
      public SuspendableObject,
      private WebMediaPlayerClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(HTMLMediaElement);
  USING_PRE_FINALIZER(HTMLMediaElement, Dispose);

 public:
  static MIMETypeRegistry::SupportsType GetSupportsType(const ContentType&);

  enum class RecordMetricsBehavior { kDoNotRecord, kDoRecord };

  static void SetMediaStreamRegistry(URLRegistry*);
  static bool IsMediaStreamURL(const String& url);
  static bool IsHLSURL(const KURL&);

  // If HTMLMediaElement is using MediaTracks (either placeholder or provided
  // by the page).
  static bool MediaTracksEnabledInternally();

  // Notify the HTMLMediaElement that the media controls settings have changed
  // for the given document.
  static void OnMediaControlsEnabledChange(Document*);

  DECLARE_VIRTUAL_TRACE();

  DECLARE_VIRTUAL_TRACE_WRAPPERS();

  void ClearWeakMembers(Visitor*);
  WebMediaPlayer* GetWebMediaPlayer() const { return web_media_player_.get(); }

  // Returns true if the loaded media has a video track.
  // Note that even an audio element can have video track in cases such as
  // <audio src="video.webm">, in which case this function will return true.
  bool HasVideo() const;
  // Returns true if loaded media has an audio track.
  bool HasAudio() const;

  bool SupportsSave() const;

  WebLayer* PlatformLayer() const;

  enum DelayedActionType {
    kLoadMediaResource = 1 << 0,
    kLoadTextTrackResource = 1 << 1
  };
  void ScheduleTextTrackResourceLoad();

  bool HasRemoteRoutes() const;
  bool IsPlayingRemotely() const { return playing_remotely_; }

  // error state
  MediaError* error() const;

  // network state
  void SetSrc(const AtomicString&);
  const KURL& currentSrc() const { return current_src_; }
  void SetSrcObject(MediaStreamDescriptor*);
  MediaStreamDescriptor* GetSrcObject() const { return src_object_.Get(); }

  enum NetworkState {
    kNetworkEmpty,
    kNetworkIdle,
    kNetworkLoading,
    kNetworkNoSource
  };
  NetworkState getNetworkState() const;

  String preload() const;
  void setPreload(const AtomicString&);
  WebMediaPlayer::Preload PreloadType() const;
  String EffectivePreload() const;
  WebMediaPlayer::Preload EffectivePreloadType() const;

  TimeRanges* buffered() const;
  void load();
  String canPlayType(const String& mime_type) const;

  // ready state
  enum ReadyState {
    kHaveNothing,
    kHaveMetadata,
    kHaveCurrentData,
    kHaveFutureData,
    kHaveEnoughData
  };
  ReadyState getReadyState() const;
  bool seeking() const;

  // playback state
  double currentTime() const;
  void setCurrentTime(double);
  double duration() const;
  bool paused() const;
  double defaultPlaybackRate() const;
  void setDefaultPlaybackRate(double);
  double playbackRate() const;
  void setPlaybackRate(double);
  void UpdatePlaybackRate();
  TimeRanges* played();
  TimeRanges* seekable() const;
  bool ended() const;
  bool Autoplay() const;
  bool ShouldAutoplay();
  bool Loop() const;
  void SetLoop(bool);
  ScriptPromise playForBindings(ScriptState*);
  Nullable<ExceptionCode> Play();
  void pause();
  void RequestRemotePlayback();
  void RequestRemotePlaybackControl();
  void RequestRemotePlaybackStop();

  // Detached view / VR player
  void OperaReleaseDetachedView();
  void OperaInvokeDetachedViewAction(const String& action);
  bool OperaDetachedView();
  void DetachedViewStateChanged(bool) override;
  void DetachedViewActionTriggered(const WebString& action) override;
  void MuteStateChanged(bool) override;
  void PlaybackStateToggled() override;
  void VRPlayerStateChanged(bool) override;
  void VRPlayerErrorOccured(uint16_t type) override;

  // statistics
  unsigned webkitAudioDecodedByteCount() const;
  unsigned webkitVideoDecodedByteCount() const;

  // media source extensions
  void CloseMediaSource();
  void DurationChanged(double duration, bool request_seek);

  // controls
  bool ShouldShowControls(
      const RecordMetricsBehavior = RecordMetricsBehavior::kDoNotRecord) const;
  DOMTokenList* controlsList() const;
  HTMLMediaElementControlsList* ControlsListInternal() const;
  double volume() const;
  void setVolume(double, ExceptionState& = ASSERT_NO_EXCEPTION);
  bool muted() const;
  void setMuted(bool);

  void TogglePlayState();

  AudioTrackList& audioTracks();
  void AudioTrackChanged(AudioTrack*);

  VideoTrackList& videoTracks();
  void SelectedVideoTrackChanged(VideoTrack*);

  TextTrack* addTextTrack(const AtomicString& kind,
                          const AtomicString& label,
                          const AtomicString& language,
                          ExceptionState&);

  TextTrackList* textTracks();
  CueTimeline& GetCueTimeline();

  void addTextTrack(TextTrack*);
  void RemoveTextTrack(TextTrack*);
  void TextTracksChanged();
  void NotifyMediaPlayerOfTextTrackChanges();

  // Implements the "forget the media element's media-resource-specific tracks"
  // algorithm in the HTML5 spec.
  void ForgetResourceSpecificTracks();

  void DidAddTrackElement(HTMLTrackElement*);
  void DidRemoveTrackElement(HTMLTrackElement*);

  void HonorUserPreferencesForAutomaticTextTrackSelection();

  bool TextTracksAreReady() const;
  void ConfigureTextTrackDisplay();
  void UpdateTextTrackDisplay();
  double LastSeekTime() const { return last_seek_time_; }
  void TextTrackReadyStateChanged(TextTrack*);

  void TextTrackModeChanged(TextTrack*);
  void DisableAutomaticTextTrackSelection();

  // EventTarget function.
  // Both Node (via HTMLElement) and SuspendableObject define this method, which
  // causes an ambiguity error at compile time. This class's constructor
  // ensures that both implementations return document, so return the result
  // of one of them here.
  using HTMLElement::GetExecutionContext;

  bool HasSingleSecurityOrigin() const {
    return GetWebMediaPlayer() &&
           GetWebMediaPlayer()->HasSingleSecurityOrigin();
  }

  bool IsFullscreen() const;
  void DidEnterFullscreen();
  void DidExitFullscreen();
  virtual bool UsesOverlayFullscreenVideo() const { return false; }

  bool HasClosedCaptions() const;
  bool TextTracksVisible() const;

  static void SetTextTrackKindUserPreferenceForAllMediaElements(Document*);
  void AutomaticTrackSelectionForUpdatedUserPreference();

  // Returns the MediaControls, or null if they have not been added yet.
  // Note that this can be non-null even if there is no controls attribute.
  MediaControls* GetMediaControls() const;

  // Notifies the media element that the media controls became visible, so
  // that text track layout may be updated to avoid overlapping them.
  void MediaControlsDidBecomeVisible();

  void SourceWasRemoved(HTMLSourceElement*);
  void SourceWasAdded(HTMLSourceElement*);

  // ScriptWrappable functions.
  bool HasPendingActivity() const override;

  AudioSourceProviderClient* AudioSourceNode() { return audio_source_node_; }
  void SetAudioSourceNode(AudioSourceProviderClient*);

  AudioSourceProvider& GetAudioSourceProvider() {
    return audio_source_provider_;
  }

  enum InvalidURLAction { kDoNothing, kComplain };
  bool IsSafeToLoadURL(const KURL&, InvalidURLAction);

  // Checks to see if current media data is CORS-same-origin as the
  // specified origin.
  bool IsMediaDataCORSSameOrigin(SecurityOrigin*) const;

  // Returns this media element is in a cross-origin frame.
  bool IsInCrossOriginFrame() const;

  void ScheduleEvent(Event*);
  void ScheduleTimeupdateEvent(bool periodic_event);

  // Returns the "effective media volume" value as specified in the HTML5 spec.
  double EffectiveMediaVolume() const;

  // Predicates also used when dispatching wrapper creation (cf.
  // [SpecialWrapFor] IDL attribute usage.)
  virtual bool IsHTMLAudioElement() const { return false; }
  virtual bool IsHTMLVideoElement() const { return false; }

  void VideoWillBeDrawnToCanvas() const;

  const WebRemotePlaybackClient* RemotePlaybackClient() const {
    return remote_playback_client_;
  }

  bool IsVideoDetachAllowed() const override;
  void RequestDetachedView() override;
  void ReleaseDetachedView() override;
  bool HasDetachedView() override;
  void InvokeDetachedViewAction(const String& action) override;
  void UpdateDetachedViewSubtitle(const String& text) override;
  void RequestVRPlayback() override;
  void ExitVRPlayback() override;
  bool HasVRPlayback() override;
  bool IsVRPlaybackAllowed() override;

  const AutoplayPolicy& GetAutoplayPolicy() const { return *autoplay_policy_; }

  WebMediaPlayer::LoadType GetLoadType() const;

 protected:
  HTMLMediaElement(const QualifiedName&, Document&);
  ~HTMLMediaElement() override;
  void Dispose();

  void ParseAttribute(const AttributeModificationParams&) override;
  void FinishParsingChildren() final;
  bool IsURLAttribute(const Attribute&) const override;
  void AttachLayoutTree(AttachContext&) override;

  InsertionNotificationRequest InsertedInto(ContainerNode*) override;
  void RemovedFrom(ContainerNode*) override;

  void DidMoveToNewDocument(Document& old_document) override;
  virtual KURL PosterImageURL() const { return KURL(); }

  enum DisplayMode { kUnknown, kPoster, kVideo };
  DisplayMode GetDisplayMode() const { return display_mode_; }
  virtual void SetDisplayMode(DisplayMode mode) { display_mode_ = mode; }

  // Assert the correct order of the children in shadow dom when DCHECK is on.
  static void AssertShadowRootChildren(ShadowRoot&);

 private:
  // Friend class for testing.
  friend class MediaElementFillingViewportTest;

  void ResetMediaPlayerAndMediaSource();

  bool AlwaysCreateUserAgentShadowRoot() const final { return true; }
  bool AreAuthorShadowsAllowed() const final { return false; }

  bool SupportsFocus() const final;
  bool IsMouseFocusable() const final;
  bool LayoutObjectIsNeeded(const ComputedStyle&) override;
  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;
  void DidNotifySubtreeInsertionsToDocument() override;
  void DidRecalcStyle() final;

  bool CanStartSelection(SelectionStartPolicy) const override { return false; }

  bool IsInteractiveContent() const final;

  // SuspendableObject functions.
  void ContextDestroyed(ExecutionContext*) override;

  virtual void UpdateDisplayState() {}

  void SetReadyState(ReadyState);
  void SetNetworkState(WebMediaPlayer::NetworkState);

  // WebMediaPlayerClient implementation.
  void NetworkStateChanged() final;
  void ReadyStateChanged() final;
  void TimeChanged() final;
  void Repaint() final;
  void DurationChanged() final;
  void SizeChanged() final;
  void PlaybackStateChanged() final;

  void SetWebLayer(WebLayer*) final;
  WebMediaPlayer::TrackId AddAudioTrack(const WebString&,
                                        WebMediaPlayerClient::AudioTrackKind,
                                        const WebString&,
                                        const WebString&,
                                        bool) final;
  void RemoveAudioTrack(WebMediaPlayer::TrackId) final;
  WebMediaPlayer::TrackId AddVideoTrack(const WebString&,
                                        WebMediaPlayerClient::VideoTrackKind,
                                        const WebString&,
                                        const WebString&,
                                        bool) final;
  void RemoveVideoTrack(WebMediaPlayer::TrackId) final;
  void AddTextTrack(WebInbandTextTrack*) final;
  void RemoveTextTrack(WebInbandTextTrack*) final;
  void MediaSourceOpened(WebMediaSource*) final;
  void RequestSeek(double) final;
  void RemoteRouteAvailabilityChanged(WebRemotePlaybackAvailability) final;
  void ConnectedToRemoteDevice() final;
  void DisconnectedFromRemoteDevice() final;
  void CancelledRemotePlaybackRequest() final;
  void RemotePlaybackStarted() final;
  void RemotePlaybackCompatibilityChanged(const WebURL&,
                                          bool is_compatible) final;
  void OnBecamePersistentVideo(bool) override {}
  bool HasSelectedVideoTrack() final;
  WebMediaPlayer::TrackId GetSelectedVideoTrackId() final;
  bool IsAutoplayingMuted() final;
  void ActivateViewportIntersectionMonitoring(bool) final;
  bool HasNativeControls() final;
  bool IsAudioElement() final;
  WebMediaPlayer::DisplayType DisplayType() const override;
  WebRemotePlaybackClient* RemotePlaybackClient() final {
    return remote_playback_client_;
  }
  gfx::ColorSpace TargetColorSpace() override;

  void LoadTimerFired(TimerBase*);
  void ProgressEventTimerFired(TimerBase*);
  void PlaybackProgressTimerFired(TimerBase*);
  void CheckViewportIntersectionTimerFired(TimerBase*);
  void StartPlaybackProgressTimer();
  void StartProgressEventTimer();
  void StopPeriodicTimers();

  void Seek(double time);
  void FinishSeek();
  void CheckIfSeekNeeded();
  void AddPlayedRange(double start, double end);

  // FIXME: Rename to scheduleNamedEvent for clarity.
  void ScheduleEvent(const AtomicString& event_name);

  // loading
  void InvokeLoadAlgorithm();
  void InvokeResourceSelectionAlgorithm();
  void LoadInternal();
  void SelectMediaResource();
  void LoadResource(const WebMediaPlayerSource&, const String& content_type);
  void StartPlayerLoad();
  void SetPlayerPreload();
  void ScheduleNextSourceChild();
  void LoadSourceFromObject();
  void LoadSourceFromAttribute();
  void LoadNextSourceChild();
  void ClearMediaPlayer();
  void ClearMediaPlayerAndAudioSourceProviderClientWithoutLocking();
  bool HavePotentialSourceChild();
  void NoneSupported(const String&);
  void MediaEngineError(MediaError*);
  void CancelPendingEventsAndCallbacks();
  void WaitForSourceChange();
  void SetIgnorePreloadNone();

  KURL SelectNextSourceChild(String* content_type, InvalidURLAction);

  void MediaLoadingFailed(WebMediaPlayer::NetworkState, const String&);

  // deferred loading (preload=none)
  bool LoadIsDeferred() const;
  void DeferLoad();
  void CancelDeferredLoad();
  void StartDeferredLoad();
  void ExecuteDeferredLoad();
  void DeferredLoadTimerFired(TimerBase*);

  void MarkCaptionAndSubtitleTracksAsUnconfigured();

  // This does not check user gesture restrictions.
  void PlayInternal();

  // This does not stop autoplay visibility observation.
  void PauseInternal();

  void AllowVideoRendering();

  void UpdateVolume();
  void UpdatePlayState();
  bool PotentiallyPlaying() const;
  bool StoppedDueToErrors() const;
  bool CouldPlayIfEnoughData() const;

  // Generally the presence of the loop attribute should be considered to mean
  // playback has not "ended", as "ended" and "looping" are mutually exclusive.
  // See
  // https://html.spec.whatwg.org/multipage/embedded-content.html#ended-playback
  enum class LoopCondition { kIncluded, kIgnored };
  bool EndedPlayback(LoopCondition = LoopCondition::kIncluded) const;

  void SetShouldDelayLoadEvent(bool);

  double EarliestPossiblePosition() const;
  double CurrentPlaybackPosition() const;
  double OfficialPlaybackPosition() const;
  void SetOfficialPlaybackPosition(double) const;
  void RequireOfficialPlaybackPositionUpdate() const;

  void EnsureMediaControls();
  void UpdateControlsVisibility();

  TextTrackContainer& EnsureTextTrackContainer();

  void ChangeNetworkStateFromLoadingToIdle();

  WebMediaPlayer::CORSMode CorsMode() const;

  // Returns the "direction of playback" value as specified in the HTML5 spec.
  enum DirectionOfPlayback { kBackward, kForward };
  DirectionOfPlayback GetDirectionOfPlayback() const;

  // Creates placeholder AudioTrack and/or VideoTrack objects when
  // WebMemediaPlayer objects advertise they have audio and/or video, but don't
  // explicitly signal them via addAudioTrack() and addVideoTrack().
  // FIXME: Remove this once all WebMediaPlayer implementations properly report
  // their track info.
  void CreatePlaceholderTracksIfNecessary();

  // Sets the selected/enabled tracks if they aren't set before we initially
  // transition to kHaveMetadata.
  void SelectInitialTracksIfNecessary();

  void SetNetworkState(NetworkState);

  void AudioTracksTimerFired(TimerBase*);

  void ScheduleResolvePlayPromises();
  void ScheduleRejectPlayPromises(ExceptionCode);
  void ScheduleNotifyPlaying();
  void ResolveScheduledPlayPromises();
  void RejectScheduledPlayPromises();
  void RejectPlayPromises(ExceptionCode, const String&);
  void RejectPlayPromisesInternal(ExceptionCode, const String&);

  void OperaRequestDetachedViewInternal();
  bool OperaGetDetachedViewControls(AtomicString* controls,
                                    AtomicString* title);
  void VRPlayerStateChangeNeeded(TimerBase*);

  EnumerationHistogram& ShowControlsHistogram() const;

  TaskRunnerTimer<HTMLMediaElement> load_timer_;
  TaskRunnerTimer<HTMLMediaElement> progress_event_timer_;
  TaskRunnerTimer<HTMLMediaElement> playback_progress_timer_;
  TaskRunnerTimer<HTMLMediaElement> audio_tracks_timer_;
  TaskRunnerTimer<HTMLMediaElement> check_viewport_intersection_timer_;
  TaskRunnerTimer<HTMLMediaElement> vr_state_timer_;

  Member<TimeRanges> played_time_ranges_;
  Member<MediaElementEventQueue> async_event_queue_;

  double playback_rate_;
  double default_playback_rate_;
  NetworkState network_state_;
  ReadyState ready_state_;
  ReadyState ready_state_maximum_;
  KURL current_src_;
  Member<MediaStreamDescriptor> src_object_;

  Member<MediaError> error_;

  double volume_;
  double last_seek_time_;

  double previous_progress_time_;

  // Cached duration to suppress duplicate events if duration unchanged.
  double duration_;

  // The last time a timeupdate event was sent (wall clock).
  double last_time_update_event_wall_time_;

  // The last time a timeupdate event was sent in movie time.
  double last_time_update_event_media_time_;

  // The default playback start position.
  double default_playback_start_position_;

  // Loading state.
  enum LoadState {
    kWaitingForSource,
    kLoadingFromSrcObject,
    kLoadingFromSrcAttr,
    kLoadingFromSourceElement
  };
  LoadState load_state_;
  Member<HTMLSourceElement> current_source_node_;
  Member<Node> next_child_node_to_consider_;

  // "Deferred loading" state (for preload=none).
  enum DeferredLoadState {
    // The load is not deferred.
    kNotDeferred,
    // The load is deferred, and waiting for the task to set the
    // delaying-the-load-event flag (to false).
    kWaitingForStopDelayingLoadEventTask,
    // The load is the deferred, and waiting for a triggering event.
    kWaitingForTrigger,
    // The load is deferred, and waiting for the task to set the
    // delaying-the-load-event flag, after which the load will be executed.
    kExecuteOnStopDelayingLoadEventTask
  };
  DeferredLoadState deferred_load_state_;
  TaskRunnerTimer<HTMLMediaElement> deferred_load_timer_;

  std::unique_ptr<WebMediaPlayer> web_media_player_;
  WebLayer* web_layer_;

  DisplayMode display_mode_;

  Member<HTMLMediaSource> media_source_;

  // Stores "official playback position", updated periodically from "current
  // playback position". Official playback position should not change while
  // scripts are running. See setOfficialPlaybackPosition().
  mutable double official_playback_position_;
  mutable bool official_playback_position_needs_update_;

  double fragment_end_time_;

  typedef unsigned PendingActionFlags;
  PendingActionFlags pending_action_flags_;

  // FIXME: HTMLMediaElement has way too many state bits.
  bool playing_ : 1;
  bool should_delay_load_event_ : 1;
  bool have_fired_loaded_data_ : 1;
  bool can_autoplay_ : 1;
  bool muted_ : 1;
  bool paused_ : 1;
  bool seeking_ : 1;

  // data has not been loaded since sending a "stalled" event
  bool sent_stalled_event_ : 1;

  bool ignore_preload_none_ : 1;

  bool text_tracks_visible_ : 1;
  bool should_perform_automatic_track_selection_ : 1;

  bool tracks_are_ready_ : 1;
  bool processing_preference_change_ : 1;
  bool playing_remotely_ : 1;
  // Whether this element is in overlay fullscreen mode.
  bool in_overlay_fullscreen_video_ : 1;
  bool previous_player_had_detached_view_ : 1;
  bool previous_player_had_vr_player_ : 1;

  bool mostly_filling_viewport_ : 1;

  TraceWrapperMember<AudioTrackList> audio_tracks_;
  TraceWrapperMember<VideoTrackList> video_tracks_;
  TraceWrapperMember<TextTrackList> text_tracks_;
  HeapVector<Member<TextTrack>> text_tracks_when_resource_selection_began_;

  Member<CueTimeline> cue_timeline_;

  HeapVector<Member<ScriptPromiseResolver>> play_promise_resolvers_;
  TaskHandle play_promise_resolve_task_handle_;
  TaskHandle play_promise_reject_task_handle_;
  HeapVector<Member<ScriptPromiseResolver>> play_promise_resolve_list_;
  HeapVector<Member<ScriptPromiseResolver>> play_promise_reject_list_;
  ExceptionCode play_promise_error_code_;

  // This is a weak reference, since audio_source_node_ holds a reference to us.
  // TODO(Oilpan): Consider making this a strongly traced pointer with oilpan
  // where strong cycles are not a problem.
  GC_PLUGIN_IGNORE("http://crbug.com/404577")
  WeakMember<AudioSourceProviderClient> audio_source_node_;

  // AudioClientImpl wraps an AudioSourceProviderClient.
  // When the audio format is known, Chromium calls setFormat().
  class AudioClientImpl final
      : public GarbageCollectedFinalized<AudioClientImpl>,
        public WebAudioSourceProviderClient {
   public:
    explicit AudioClientImpl(AudioSourceProviderClient* client)
        : client_(client) {}

    ~AudioClientImpl() override {}

    // WebAudioSourceProviderClient
    void SetFormat(size_t number_of_channels, float sample_rate) override;

    DECLARE_TRACE();

   private:
    Member<AudioSourceProviderClient> client_;
  };

  // AudioSourceProviderImpl wraps a WebAudioSourceProvider.
  // provideInput() calls into Chromium to get a rendered audio stream.
  class AudioSourceProviderImpl final : public AudioSourceProvider {
    DISALLOW_NEW();

   public:
    AudioSourceProviderImpl() : web_audio_source_provider_(nullptr) {}

    ~AudioSourceProviderImpl() override {}

    // Wraps the given WebAudioSourceProvider.
    void Wrap(WebAudioSourceProvider*);

    // AudioSourceProvider
    void SetClient(AudioSourceProviderClient*) override;
    void ProvideInput(AudioBus*, size_t frames_to_process) override;

    DECLARE_TRACE();

   private:
    WebAudioSourceProvider* web_audio_source_provider_;
    Member<AudioClientImpl> client_;
    Mutex provide_input_lock;
  };

  AudioSourceProviderImpl audio_source_provider_;

  WebString content_type_;

  friend class AutoplayPolicy;
  friend class AutoplayUmaHelperTest;
  friend class Internals;
  friend class TrackDisplayUpdateScope;
  friend class MediaControlsImplTest;
  friend class HTMLMediaElementTest;
  friend class HTMLMediaElementEventListenersTest;
  friend class HTMLVideoElement;
  friend class MediaControlInputElementTest;
  friend class MediaControlsOrientationLockDelegateTest;
  friend class MediaControlsRotateToFullscreenDelegateTest;

  Member<AutoplayPolicy> autoplay_policy_;

  WebRemotePlaybackClient* remote_playback_client_;

  IntRect current_intersect_rect_;

  Member<MediaControls> media_controls_;
  Member<HTMLMediaElementControlsList> controls_list_;

  static URLRegistry* media_stream_registry_;
};

inline bool IsHTMLMediaElement(const HTMLElement& element) {
  return isHTMLAudioElement(element) || isHTMLVideoElement(element);
}

DEFINE_HTMLELEMENT_TYPE_CASTS_WITH_FUNCTION(HTMLMediaElement);

}  // namespace blink

#endif  // HTMLMediaElement_h
