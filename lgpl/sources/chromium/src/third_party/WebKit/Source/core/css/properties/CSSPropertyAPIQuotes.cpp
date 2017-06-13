// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPIQuotes.h"

#include "core/css/CSSStringValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"

namespace blink {

const CSSValue* CSSPropertyAPIQuotes::parseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext* context) {
  if (range.peek().id() == CSSValueNone)
    return CSSPropertyParserHelpers::consumeIdent(range);
  CSSValueList* values = CSSValueList::createSpaceSeparated();
  while (!range.atEnd()) {
    CSSStringValue* parsedValue =
        CSSPropertyParserHelpers::consumeString(range);
    if (!parsedValue)
      return nullptr;
    values->append(*parsedValue);
  }
  if (values->length() && values->length() % 2 == 0)
    return values;
  return nullptr;
}

}  // namespace blink