// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/MediaValuesInitialViewport.h"

#include "core/frame/LocalFrameView.h"
#include "core/testing/PageTestBase.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class MediaValuesInitialViewportTest : public PageTestBase {
 private:
  void SetUp() override {
    PageTestBase::SetUp(IntSize(320, 480));
    GetDocument().View()->SetInitialViewportSize(IntSize(320, 480));
  }
};

TEST_F(MediaValuesInitialViewportTest, InitialViewportSize) {
  LocalFrameView* view = GetDocument().View();
  ASSERT_TRUE(view);
  EXPECT_TRUE(view->LayoutSizeFixedToFrameSize());

  MediaValues* media_values =
      MediaValuesInitialViewport::Create(*GetDocument().GetFrame());
  EXPECT_EQ(320, media_values->ViewportWidth());
  EXPECT_EQ(480, media_values->ViewportHeight());

  view->SetLayoutSizeFixedToFrameSize(false);
  view->SetLayoutSize(IntSize(800, 600));
  EXPECT_EQ(320, media_values->ViewportWidth());
  EXPECT_EQ(480, media_values->ViewportHeight());
}

}  // namespace blink
