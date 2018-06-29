// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"

namespace blink {

WebGraphicsContext3DProviderWrapper::~WebGraphicsContext3DProviderWrapper() {
  for (auto& observer : observers_)
    observer.OnContextDestroyed();
}

void WebGraphicsContext3DProviderWrapper::AddObserver(
    DestructionObserver* obs) {
  observers_.AddObserver(obs);
}

void WebGraphicsContext3DProviderWrapper::RemoveObserver(
    DestructionObserver* obs) {
  observers_.RemoveObserver(obs);
}

}  // namespace blink
