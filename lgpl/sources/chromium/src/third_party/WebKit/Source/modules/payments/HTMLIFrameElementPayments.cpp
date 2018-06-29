// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/HTMLIFrameElementPayments.h"

#include "core/dom/QualifiedName.h"
#include "core/html/HTMLIFrameElement.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

HTMLIFrameElementPayments::HTMLIFrameElementPayments() = default;

// static
const char HTMLIFrameElementPayments::kSupplementName[] =
    "HTMLIFrameElementPayments";

// static
bool HTMLIFrameElementPayments::FastHasAttribute(
    const QualifiedName& name,
    const HTMLIFrameElement& element) {
  DCHECK(name == HTMLNames::allowpaymentrequestAttr);
  return element.FastHasAttribute(name);
}

// static
void HTMLIFrameElementPayments::SetBooleanAttribute(const QualifiedName& name,
                                                    HTMLIFrameElement& element,
                                                    bool value) {
  DCHECK(name == HTMLNames::allowpaymentrequestAttr);
  element.SetBooleanAttribute(name, value);
}

// static
HTMLIFrameElementPayments& HTMLIFrameElementPayments::From(
    HTMLIFrameElement& iframe) {
  HTMLIFrameElementPayments* supplement =
      Supplement<HTMLIFrameElement>::From<HTMLIFrameElementPayments>(iframe);
  if (!supplement) {
    supplement = new HTMLIFrameElementPayments();
    ProvideTo(iframe, supplement);
  }
  return *supplement;
}

// static
bool HTMLIFrameElementPayments::AllowPaymentRequest(
    HTMLIFrameElement& element) {
  return RuntimeEnabledFeatures::PaymentRequestEnabled() &&
         element.FastHasAttribute(HTMLNames::allowpaymentrequestAttr);
}

void HTMLIFrameElementPayments::Trace(blink::Visitor* visitor) {
  Supplement<HTMLIFrameElement>::Trace(visitor);
}

}  // namespace blink
