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

#include "core/frame/FrameTestHelpers.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/frame/PageScaleConstraints.h"
#include "core/frame/Settings.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/page/Page.h"
#include "platform/Length.h"
#include "platform/geometry/IntPoint.h"
#include "platform/geometry/IntRect.h"
#include "platform/geometry/IntSize.h"
#include "platform/scroll/ScrollbarTheme.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/Platform.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "public/web/WebConsoleMessage.h"
#include "public/web/WebFrame.h"
#include "public/web/WebLocalFrame.h"
#include "public/web/WebScriptSource.h"
#include "public/web/WebSettings.h"
#include "public/web/WebViewClient.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

using blink::testing::RunPendingTasks;

class ViewportTest : public ::testing::Test {
 protected:
  ViewportTest()
      : base_url_("http://www.test.com/"), chrome_url_("chrome://") {}

  ~ViewportTest() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void RegisterMockedHttpURLLoad(const std::string& file_name) {
    URLTestHelpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(base_url_), testing::CoreTestDataPath(),
        WebString::FromUTF8(file_name));
  }

  void RegisterMockedChromeURLLoad(const std::string& file_name) {
    URLTestHelpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(chrome_url_), testing::CoreTestDataPath(),
        WebString::FromUTF8(file_name));
  }

  void ExecuteScript(WebLocalFrame* frame, const WebString& code) {
    frame->ExecuteScript(WebScriptSource(code));
    RunPendingTasks();
  }

  std::string base_url_;
  std::string chrome_url_;
};

static void SetViewportSettings(WebSettings* settings) {
  settings->SetViewportEnabled(true);
  settings->SetViewportMetaEnabled(true);
  settings->SetMainFrameResizesAreOrientationChanges(true);
}

static PageScaleConstraints RunViewportTest(Page* page,
                                            int initial_width,
                                            int initial_height) {
  IntSize initial_viewport_size(initial_width, initial_height);
  ToLocalFrame(page->MainFrame())
      ->View()
      ->SetFrameRect(IntRect(IntPoint::Zero(), initial_viewport_size));
  ViewportDescription description = page->GetViewportDescription();
  PageScaleConstraints constraints = description.Resolve(
      FloatSize(initial_viewport_size), Length(980, blink::kFixed));

  constraints.FitToContentsWidth(constraints.layout_size.Width(),
                                 initial_width);
  constraints.ResolveAutoInitialScale();
  return constraints;
}

TEST_F(ViewportTest, viewport1) {
  RegisterMockedHttpURLLoad("viewport/viewport-1.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-1.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport2) {
  RegisterMockedHttpURLLoad("viewport/viewport-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-2.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(0.32f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.32f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport3) {
  RegisterMockedHttpURLLoad("viewport/viewport-3.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-3.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport4) {
  RegisterMockedHttpURLLoad("viewport/viewport-4.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-4.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(160, constraints.layout_size.Width());
  EXPECT_EQ(176, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport5) {
  RegisterMockedHttpURLLoad("viewport/viewport-5.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-5.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport6) {
  RegisterMockedHttpURLLoad("viewport/viewport-6.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-6.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(200, constraints.layout_size.Width());
  EXPECT_EQ(220, constraints.layout_size.Height());
  EXPECT_NEAR(1.6f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.6f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport7) {
  RegisterMockedHttpURLLoad("viewport/viewport-7.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-7.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport8) {
  RegisterMockedHttpURLLoad("viewport/viewport-8.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-8.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport9) {
  RegisterMockedHttpURLLoad("viewport/viewport-9.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-9.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport10) {
  RegisterMockedHttpURLLoad("viewport/viewport-10.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-10.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport11) {
  RegisterMockedHttpURLLoad("viewport/viewport-11.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-11.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.32f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.32f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport12) {
  RegisterMockedHttpURLLoad("viewport/viewport-12.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-12.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport13) {
  RegisterMockedHttpURLLoad("viewport/viewport-13.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-13.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport14) {
  RegisterMockedHttpURLLoad("viewport/viewport-14.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-14.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport15) {
  RegisterMockedHttpURLLoad("viewport/viewport-15.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-15.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport16) {
  RegisterMockedHttpURLLoad("viewport/viewport-16.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-16.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport17) {
  RegisterMockedHttpURLLoad("viewport/viewport-17.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-17.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport18) {
  RegisterMockedHttpURLLoad("viewport/viewport-18.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-18.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport19) {
  RegisterMockedHttpURLLoad("viewport/viewport-19.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-19.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(160, constraints.layout_size.Width());
  EXPECT_EQ(176, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport20) {
  RegisterMockedHttpURLLoad("viewport/viewport-20.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-20.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport21) {
  RegisterMockedHttpURLLoad("viewport/viewport-21.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-21.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport22) {
  RegisterMockedHttpURLLoad("viewport/viewport-22.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-22.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport23) {
  RegisterMockedHttpURLLoad("viewport/viewport-23.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-23.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(3.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(3.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport24) {
  RegisterMockedHttpURLLoad("viewport/viewport-24.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-24.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(4.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(4.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(4.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport25) {
  RegisterMockedHttpURLLoad("viewport/viewport-25.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-25.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport26) {
  RegisterMockedHttpURLLoad("viewport/viewport-26.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-26.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(8.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(8.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(9.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport27) {
  RegisterMockedHttpURLLoad("viewport/viewport-27.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-27.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.32f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.32f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport28) {
  RegisterMockedHttpURLLoad("viewport/viewport-28.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-28.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(352, constraints.layout_size.Width());
  EXPECT_NEAR(387.2, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(0.91f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.91f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport29) {
  RegisterMockedHttpURLLoad("viewport/viewport-29.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-29.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(700, constraints.layout_size.Width());
  EXPECT_EQ(770, constraints.layout_size.Height());
  EXPECT_NEAR(0.46f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.46f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport30) {
  RegisterMockedHttpURLLoad("viewport/viewport-30.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-30.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(200, constraints.layout_size.Width());
  EXPECT_EQ(220, constraints.layout_size.Height());
  EXPECT_NEAR(1.6f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.6f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport31) {
  RegisterMockedHttpURLLoad("viewport/viewport-31.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-31.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(700, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport32) {
  RegisterMockedHttpURLLoad("viewport/viewport-32.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-32.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(200, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport33) {
  RegisterMockedHttpURLLoad("viewport/viewport-33.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-33.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport34) {
  RegisterMockedHttpURLLoad("viewport/viewport-34.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-34.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport35) {
  RegisterMockedHttpURLLoad("viewport/viewport-35.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-35.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport36) {
  RegisterMockedHttpURLLoad("viewport/viewport-36.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-36.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_NEAR(636.36, constraints.layout_size.Width(), 0.01f);
  EXPECT_EQ(700, constraints.layout_size.Height());
  EXPECT_NEAR(1.6f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.50f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport37) {
  RegisterMockedHttpURLLoad("viewport/viewport-37.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-37.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport38) {
  RegisterMockedHttpURLLoad("viewport/viewport-38.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-38.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport39) {
  RegisterMockedHttpURLLoad("viewport/viewport-39.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-39.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(200, constraints.layout_size.Width());
  EXPECT_EQ(700, constraints.layout_size.Height());
  EXPECT_NEAR(1.6f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.6f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport40) {
  RegisterMockedHttpURLLoad("viewport/viewport-40.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-40.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(700, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(0.46f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.46f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport41) {
  RegisterMockedHttpURLLoad("viewport/viewport-41.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-41.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1000, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.32f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport42) {
  RegisterMockedHttpURLLoad("viewport/viewport-42.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-42.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(1000, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport43) {
  RegisterMockedHttpURLLoad("viewport/viewport-43.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-43.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport44) {
  RegisterMockedHttpURLLoad("viewport/viewport-44.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-44.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(10000, constraints.layout_size.Width());
  EXPECT_EQ(10000, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport45) {
  RegisterMockedHttpURLLoad("viewport/viewport-45.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-45.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(3200, constraints.layout_size.Width());
  EXPECT_EQ(3520, constraints.layout_size.Height());
  EXPECT_NEAR(0.1f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.1f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(0.1f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport46) {
  RegisterMockedHttpURLLoad("viewport/viewport-46.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-46.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(32, constraints.layout_size.Width());
  EXPECT_NEAR(35.2, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport47) {
  RegisterMockedHttpURLLoad("viewport/viewport-47.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-47.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(3000, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport48) {
  RegisterMockedHttpURLLoad("viewport/viewport-48.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-48.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(3000, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport49) {
  RegisterMockedHttpURLLoad("viewport/viewport-49.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-49.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport50) {
  RegisterMockedHttpURLLoad("viewport/viewport-50.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-50.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport51) {
  RegisterMockedHttpURLLoad("viewport/viewport-51.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-51.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport52) {
  RegisterMockedHttpURLLoad("viewport/viewport-52.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-52.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport53) {
  RegisterMockedHttpURLLoad("viewport/viewport-53.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-53.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport54) {
  RegisterMockedHttpURLLoad("viewport/viewport-54.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-54.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport55) {
  RegisterMockedHttpURLLoad("viewport/viewport-55.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-55.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport56) {
  RegisterMockedHttpURLLoad("viewport/viewport-56.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-56.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport57) {
  RegisterMockedHttpURLLoad("viewport/viewport-57.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-57.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport58) {
  RegisterMockedHttpURLLoad("viewport/viewport-58.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-58.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(3200, constraints.layout_size.Width());
  EXPECT_EQ(3520, constraints.layout_size.Height());
  EXPECT_NEAR(0.1f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.1f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport59) {
  RegisterMockedHttpURLLoad("viewport/viewport-59.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-59.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport60) {
  RegisterMockedHttpURLLoad("viewport/viewport-60.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-60.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(32, constraints.layout_size.Width());
  EXPECT_NEAR(35.2, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport61) {
  RegisterMockedHttpURLLoad("viewport/viewport-61.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-61.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport62) {
  RegisterMockedHttpURLLoad("viewport/viewport-62.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-62.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport63) {
  RegisterMockedHttpURLLoad("viewport/viewport-63.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-63.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport64) {
  RegisterMockedHttpURLLoad("viewport/viewport-64.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-64.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport65) {
  RegisterMockedHttpURLLoad("viewport/viewport-65.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-65.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport66) {
  RegisterMockedHttpURLLoad("viewport/viewport-66.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-66.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport67) {
  RegisterMockedHttpURLLoad("viewport/viewport-67.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-67.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport68) {
  RegisterMockedHttpURLLoad("viewport/viewport-68.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-68.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport69) {
  RegisterMockedHttpURLLoad("viewport/viewport-69.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-69.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport70) {
  RegisterMockedHttpURLLoad("viewport/viewport-70.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-70.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport71) {
  RegisterMockedHttpURLLoad("viewport/viewport-71.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-71.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport72) {
  RegisterMockedHttpURLLoad("viewport/viewport-72.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-72.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport73) {
  RegisterMockedHttpURLLoad("viewport/viewport-73.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-73.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport74) {
  RegisterMockedHttpURLLoad("viewport/viewport-74.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-74.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport75) {
  RegisterMockedHttpURLLoad("viewport/viewport-75.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-75.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport76) {
  RegisterMockedHttpURLLoad("viewport/viewport-76.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-76.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(32, constraints.layout_size.Width());
  EXPECT_NEAR(35.2, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport77) {
  RegisterMockedHttpURLLoad("viewport/viewport-77.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-77.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1280, constraints.layout_size.Width());
  EXPECT_EQ(1408, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport78) {
  RegisterMockedHttpURLLoad("viewport/viewport-78.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-78.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(100, constraints.layout_size.Width());
  EXPECT_EQ(110, constraints.layout_size.Height());
  EXPECT_NEAR(3.2f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(3.2f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport79) {
  RegisterMockedHttpURLLoad("viewport/viewport-79.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-79.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport80) {
  RegisterMockedHttpURLLoad("viewport/viewport-80.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-80.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport81) {
  RegisterMockedHttpURLLoad("viewport/viewport-81.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-81.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(3000, constraints.layout_size.Width());
  EXPECT_EQ(3300, constraints.layout_size.Height());
  EXPECT_NEAR(0.25f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.25f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport82) {
  RegisterMockedHttpURLLoad("viewport/viewport-82.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-82.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport83) {
  RegisterMockedHttpURLLoad("viewport/viewport-83.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-83.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport84) {
  RegisterMockedHttpURLLoad("viewport/viewport-84.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-84.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_EQ(480, constraints.layout_size.Height());
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport85) {
  RegisterMockedHttpURLLoad("viewport/viewport-85.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-85.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(540, constraints.layout_size.Width());
  EXPECT_EQ(594, constraints.layout_size.Height());
  EXPECT_NEAR(0.59f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.59f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport86) {
  RegisterMockedHttpURLLoad("viewport/viewport-86.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-86.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_NEAR(457.14, constraints.layout_size.Width(), 0.01f);
  EXPECT_NEAR(502.86, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(0.7f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.7f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport87) {
  RegisterMockedHttpURLLoad("viewport/viewport-87.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-87.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport88) {
  RegisterMockedHttpURLLoad("viewport/viewport-88.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-88.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport90) {
  RegisterMockedHttpURLLoad("viewport/viewport-90.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-90.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(700, constraints.layout_size.Width());
  EXPECT_EQ(770, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.46f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport100) {
  RegisterMockedHttpURLLoad("viewport/viewport-100.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-100.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport101) {
  RegisterMockedHttpURLLoad("viewport/viewport-101.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-101.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport102) {
  RegisterMockedHttpURLLoad("viewport/viewport-102.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-102.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport103) {
  RegisterMockedHttpURLLoad("viewport/viewport-103.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-103.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport104) {
  RegisterMockedHttpURLLoad("viewport/viewport-104.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-104.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport105) {
  RegisterMockedHttpURLLoad("viewport/viewport-105.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-105.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport106) {
  RegisterMockedHttpURLLoad("viewport/viewport-106.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-106.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport107) {
  RegisterMockedHttpURLLoad("viewport/viewport-107.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-107.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport108) {
  RegisterMockedHttpURLLoad("viewport/viewport-108.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-108.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport109) {
  RegisterMockedHttpURLLoad("viewport/viewport-109.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-109.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport110) {
  RegisterMockedHttpURLLoad("viewport/viewport-110.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-110.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport111) {
  RegisterMockedHttpURLLoad("viewport/viewport-111.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-111.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport112) {
  RegisterMockedHttpURLLoad("viewport/viewport-112.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-112.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport113) {
  RegisterMockedHttpURLLoad("viewport/viewport-113.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-113.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport114) {
  RegisterMockedHttpURLLoad("viewport/viewport-114.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-114.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport115) {
  RegisterMockedHttpURLLoad("viewport/viewport-115.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-115.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport116) {
  RegisterMockedHttpURLLoad("viewport/viewport-116.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-116.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(400, constraints.layout_size.Width());
  EXPECT_EQ(440, constraints.layout_size.Height());
  EXPECT_NEAR(0.8f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.8f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport117) {
  RegisterMockedHttpURLLoad("viewport/viewport-117.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-117.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(400, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport118) {
  RegisterMockedHttpURLLoad("viewport/viewport-118.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-118.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport119) {
  RegisterMockedHttpURLLoad("viewport/viewport-119.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-119.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport120) {
  RegisterMockedHttpURLLoad("viewport/viewport-120.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-120.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport121) {
  RegisterMockedHttpURLLoad("viewport/viewport-121.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-121.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport122) {
  RegisterMockedHttpURLLoad("viewport/viewport-122.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-122.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport123) {
  RegisterMockedHttpURLLoad("viewport/viewport-123.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-123.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport124) {
  RegisterMockedHttpURLLoad("viewport/viewport-124.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-124.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport125) {
  RegisterMockedHttpURLLoad("viewport/viewport-125.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-125.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport126) {
  RegisterMockedHttpURLLoad("viewport/viewport-126.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-126.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport127) {
  RegisterMockedHttpURLLoad("viewport/viewport-127.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-127.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(64, constraints.layout_size.Width());
  EXPECT_NEAR(70.4, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport129) {
  RegisterMockedHttpURLLoad("viewport/viewport-129.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-129.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(123, constraints.layout_size.Width());
  EXPECT_NEAR(135.3, constraints.layout_size.Height(), 0.01f);
  EXPECT_NEAR(2.60f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.60f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport130) {
  RegisterMockedHttpURLLoad("viewport/viewport-130.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-130.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport131) {
  RegisterMockedHttpURLLoad("viewport/viewport-131.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-131.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.maximum_scale, 0.01f);
  EXPECT_FALSE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport132) {
  RegisterMockedHttpURLLoad("viewport/viewport-132.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-132.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport133) {
  RegisterMockedHttpURLLoad("viewport/viewport-133.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-133.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(10.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(10.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport134) {
  RegisterMockedHttpURLLoad("viewport/viewport-134.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-134.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(160, constraints.layout_size.Width());
  EXPECT_EQ(176, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport135) {
  RegisterMockedHttpURLLoad("viewport/viewport-135.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-135.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport136) {
  RegisterMockedHttpURLLoad("viewport/viewport-136.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-136.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport137) {
  RegisterMockedHttpURLLoad("viewport/viewport-137.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-137.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewport138) {
  RegisterMockedHttpURLLoad("viewport/viewport-138.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-138.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_NEAR(123.0f, constraints.layout_size.Width(), 0.01);
  EXPECT_NEAR(135.3f, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(2.60f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.60f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyHandheldFriendly) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-handheldfriendly.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-handheldfriendly.html", nullptr,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

static void SetQuirkViewportSettings(WebSettings* settings) {
  SetViewportSettings(settings);

  // This quirk allows content attributes of meta viewport tags to be merged.
  settings->SetViewportMetaMergeContentQuirk(true);
}

TEST_F(ViewportTest, viewportLegacyMergeQuirk1) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-merge-quirk-1.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-merge-quirk-1.html", nullptr,
      nullptr, nullptr, SetQuirkViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.maximum_scale, 0.01f);
  EXPECT_FALSE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyMergeQuirk2) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-merge-quirk-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-merge-quirk-2.html", nullptr,
      nullptr, nullptr, SetQuirkViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  // This quirk allows content attributes of meta viewport tags to be merged.
  page->GetSettings().SetViewportMetaMergeContentQuirk(true);
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(500, constraints.layout_size.Width());
  EXPECT_EQ(550, constraints.layout_size.Height());
  EXPECT_NEAR(2.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.maximum_scale, 0.01f);
  EXPECT_FALSE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyMobileOptimizedMetaWithoutContent) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-mobileoptimized.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-mobileoptimized.html", nullptr,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyMobileOptimizedMetaWith0) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-mobileoptimized-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-mobileoptimized-2.html", nullptr,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyMobileOptimizedMetaWith400) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-mobileoptimized-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-mobileoptimized-2.html", nullptr,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering2) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-2.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(300, constraints.layout_size.Width());
  EXPECT_EQ(330, constraints.layout_size.Height());
  EXPECT_NEAR(1.07f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.07f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering3) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-3.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-3.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(300, constraints.layout_size.Width());
  EXPECT_EQ(330, constraints.layout_size.Height());
  EXPECT_NEAR(1.07f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.07f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering4) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-4.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-4.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(300, constraints.layout_size.Width());
  EXPECT_EQ(330, constraints.layout_size.Height());
  EXPECT_NEAR(1.07f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.07f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering5) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-5.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-5.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering6) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-6.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-6.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering7) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-7.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-7.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(300, constraints.layout_size.Width());
  EXPECT_EQ(330, constraints.layout_size.Height());
  EXPECT_NEAR(1.07f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.07f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyOrdering8) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-8.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-8.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();

  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(300, constraints.layout_size.Width());
  EXPECT_EQ(330, constraints.layout_size.Height());
  EXPECT_NEAR(1.07f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.07f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyEmptyAtViewportDoesntOverrideViewportMeta) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-ordering-10.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-ordering-10.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 800, 600);

  EXPECT_EQ(5000, constraints.layout_size.Width());
}

TEST_F(ViewportTest, viewportLegacyDefaultValueChangedByXHTMLMP) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-xhtmlmp.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-xhtmlmp.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest,
       viewportLegacyDefaultValueChangedByXHTMLMPAndOverriddenByMeta) {
  RegisterMockedHttpURLLoad(
      "viewport/viewport-legacy-xhtmlmp-misplaced-doctype.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-xhtmlmp-misplaced-doctype.html",
      nullptr, nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyXHTMLMPOrdering) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-xhtmlmp-ordering.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-xhtmlmp-ordering.html", nullptr,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(640, constraints.layout_size.Width());
  EXPECT_EQ(704, constraints.layout_size.Height());
  EXPECT_NEAR(0.5f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.5f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLegacyXHTMLMPRemoveAndAdd) {
  RegisterMockedHttpURLLoad("viewport/viewport-legacy-xhtmlmp.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-xhtmlmp.html", nullptr, nullptr,
      nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);

  ExecuteScript(web_view_helper.LocalMainFrame(),
                "originalDoctype = document.doctype;"
                "document.removeChild(originalDoctype);");

  constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);

  ExecuteScript(web_view_helper.LocalMainFrame(),
                "document.insertBefore(originalDoctype, document.firstChild);");

  constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportLimitsAdjustedForNoUserScale) {
  RegisterMockedHttpURLLoad(
      "viewport/viewport-limits-adjusted-for-no-user-scale.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-limits-adjusted-for-no-user-scale.html",
      nullptr, nullptr, nullptr, SetViewportSettings);

  web_view_helper.WebView()->UpdateAllLifecyclePhases();
  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 10, 10);

  EXPECT_FALSE(page->GetViewportDescription().user_zoom);
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
}

TEST_F(ViewportTest, viewportLimitsAdjustedForUserScale) {
  RegisterMockedHttpURLLoad(
      "viewport/viewport-limits-adjusted-for-user-scale.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-limits-adjusted-for-user-scale.html",
      nullptr, nullptr, nullptr, SetViewportSettings);

  web_view_helper.WebView()->UpdateAllLifecyclePhases();
  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 10, 10);

  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
}

TEST_F(ViewportTest, viewportTriggersGpuRasterization) {
  FrameTestHelpers::WebViewHelper web_view_helper;

  RegisterMockedHttpURLLoad(
      "viewport/viewport-gpu-rasterization-disabled-without-viewport.html");
  web_view_helper.InitializeAndLoad(
      base_url_ +
          "viewport/viewport-gpu-rasterization-disabled-without-viewport.html",
      nullptr, nullptr, nullptr, SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_FALSE(web_view_helper.WebView()
                   ->MatchesHeuristicsForGpuRasterizationForTesting());
  // Also test that setting enableViewport to false (as on desktop Chrome)
  // supports GPU raster unconditionally.
  web_view_helper.InitializeAndLoad(
      base_url_ +
      "viewport/viewport-gpu-rasterization-disabled-without-viewport.html");
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-gpu-rasterization.html");
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-gpu-rasterization.html", nullptr, nullptr,
      nullptr, SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad(
      "viewport/viewport-gpu-rasterization-expanded-heuristics.html");
  web_view_helper.InitializeAndLoad(
      base_url_ +
          "viewport/viewport-gpu-rasterization-expanded-heuristics.html",
      nullptr, nullptr, nullptr, SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-1.html");
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-1.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-15.html");
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-15.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-130.html");
  web_view_helper.InitializeAndLoad(base_url_ + "viewport/viewport-130.html",
                                    nullptr, nullptr, nullptr,
                                    SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-legacy-handheldfriendly.html");
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-handheldfriendly.html", nullptr,
      nullptr, nullptr, SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());

  RegisterMockedHttpURLLoad("viewport/viewport-legacy-mobileoptimized.html");
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-legacy-handheldfriendly.html", nullptr,
      nullptr, nullptr, SetViewportSettings);
  web_view_helper.WebView()->Resize(WebSize(640, 480));
  EXPECT_TRUE(web_view_helper.WebView()
                  ->MatchesHeuristicsForGpuRasterizationForTesting());
}

class ConsoleMessageWebFrameClient
    : public FrameTestHelpers::TestWebFrameClient {
 public:
  virtual void DidAddMessageToConsole(const WebConsoleMessage& msg,
                                      const WebString& source_name,
                                      unsigned source_line,
                                      const WebString& stack_trace) {
    messages.push_back(msg);
  }

  Vector<WebConsoleMessage> messages;
};

TEST_F(ViewportTest, viewportWarnings1) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-1.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-1.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_TRUE(web_frame_client.messages.IsEmpty());

  EXPECT_EQ(320, constraints.layout_size.Width());
  EXPECT_EQ(352, constraints.layout_size.Height());
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(2.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings2) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-2.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-2.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1U, web_frame_client.messages.size());
  EXPECT_EQ(WebConsoleMessage::kLevelWarning,
            web_frame_client.messages[0].level);
  EXPECT_STREQ("The key \"wwidth\" is not recognized and ignored.",
               web_frame_client.messages[0].text.Utf8().c_str());

  EXPECT_EQ(980, constraints.layout_size.Width());
  EXPECT_EQ(1078, constraints.layout_size.Height());
  EXPECT_NEAR(0.33f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(0.33f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings3) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-3.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-3.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1U, web_frame_client.messages.size());
  EXPECT_EQ(WebConsoleMessage::kLevelWarning,
            web_frame_client.messages[0].level);
  EXPECT_STREQ(
      "The value \"unrecognized-width\" for key \"width\" is invalid, and has "
      "been ignored.",
      web_frame_client.messages[0].text.Utf8().c_str());

  EXPECT_NEAR(64.0f, constraints.layout_size.Width(), 0.01);
  EXPECT_NEAR(70.4f, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings4) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-4.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-4.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1U, web_frame_client.messages.size());
  EXPECT_EQ(WebConsoleMessage::kLevelWarning,
            web_frame_client.messages[0].level);
  EXPECT_STREQ(
      "The value \"123x456\" for key \"width\" was truncated to its numeric "
      "prefix.",
      web_frame_client.messages[0].text.Utf8().c_str());

  EXPECT_NEAR(123.0f, constraints.layout_size.Width(), 0.01);
  EXPECT_NEAR(135.3f, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(2.60f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(2.60f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings5) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-5.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-5.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1U, web_frame_client.messages.size());

  EXPECT_EQ(WebConsoleMessage::kLevelWarning,
            web_frame_client.messages[0].level);
  EXPECT_STREQ(
      "Error parsing a meta element's content: ';' is not a valid key-value "
      "pair separator. Please use ',' instead.",
      web_frame_client.messages[0].text.Utf8().c_str());

  EXPECT_NEAR(320.0f, constraints.layout_size.Width(), 0.01);
  EXPECT_NEAR(352.0f, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(1.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(1.0f, constraints.maximum_scale, 0.01f);
  EXPECT_FALSE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings6) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-6.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-6.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  PageScaleConstraints constraints = RunViewportTest(page, 320, 352);

  EXPECT_EQ(1U, web_frame_client.messages.size());
  EXPECT_EQ(WebConsoleMessage::kLevelWarning,
            web_frame_client.messages[0].level);
  EXPECT_STREQ(
      "The value \"\" for key \"width\" is invalid, and has been ignored.",
      web_frame_client.messages[0].text.Utf8().c_str());

  EXPECT_NEAR(64.0f, constraints.layout_size.Width(), 0.01);
  EXPECT_NEAR(70.4f, constraints.layout_size.Height(), 0.01);
  EXPECT_NEAR(5.0f, constraints.initial_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.minimum_scale, 0.01f);
  EXPECT_NEAR(5.0f, constraints.maximum_scale, 0.01f);
  EXPECT_TRUE(page->GetViewportDescription().user_zoom);
}

TEST_F(ViewportTest, viewportWarnings7) {
  ConsoleMessageWebFrameClient web_frame_client;

  RegisterMockedHttpURLLoad("viewport/viewport-warnings-7.html");

  FrameTestHelpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(
      base_url_ + "viewport/viewport-warnings-7.html", &web_frame_client,
      nullptr, nullptr, SetViewportSettings);

  Page* page = web_view_helper.WebView()->GetPage();
  RunViewportTest(page, 320, 352);

  EXPECT_EQ(0U, web_frame_client.messages.size());
}

}  // namespace blink
