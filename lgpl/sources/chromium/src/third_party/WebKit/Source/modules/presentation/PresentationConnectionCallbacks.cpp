// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/presentation/PresentationConnectionCallbacks.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/presentation/PresentationConnection.h"
#include "modules/presentation/PresentationError.h"
#include "modules/presentation/PresentationRequest.h"

namespace blink {

PresentationConnectionCallbacks::PresentationConnectionCallbacks(
    ScriptPromiseResolver* resolver,
    PresentationRequest* request)
    : resolver_(resolver), request_(request), connection_(nullptr) {
  DCHECK(resolver_);
  DCHECK(request_);
}

PresentationConnectionCallbacks::PresentationConnectionCallbacks(
    ScriptPromiseResolver* resolver,
    ControllerPresentationConnection* connection)
    : resolver_(resolver), request_(nullptr), connection_(connection) {
  DCHECK(resolver_);
  DCHECK(connection_);
}

void PresentationConnectionCallbacks::HandlePresentationResponse(
    mojom::blink::PresentationInfoPtr presentation_info,
    mojom::blink::PresentationErrorPtr error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed()) {
    return;
  }

  if (presentation_info)
    OnSuccess(*presentation_info);
  else
    OnError(*error);
}

void PresentationConnectionCallbacks::OnSuccess(
    const mojom::blink::PresentationInfo& presentation_info) {
  // Reconnect to existing connection.
  if (connection_ && connection_->GetState() ==
                         mojom::blink::PresentationConnectionState::CLOSED) {
    connection_->DidChangeState(
        mojom::blink::PresentationConnectionState::CONNECTING);
  }

  // Create a new connection.
  if (!connection_ && request_) {
    connection_ = ControllerPresentationConnection::Take(
        resolver_.Get(), presentation_info, request_);
  }

  resolver_->Resolve(connection_);
  connection_->Init();
}

void PresentationConnectionCallbacks::OnError(
    const mojom::blink::PresentationError& error) {
  resolver_->Reject(CreatePresentationError(error));
  connection_ = nullptr;
}

}  // namespace blink
