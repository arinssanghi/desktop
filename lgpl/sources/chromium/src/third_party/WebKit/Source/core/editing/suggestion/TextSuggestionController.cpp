// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/suggestion/TextSuggestionController.h"

#include "core/editing/EditingUtilities.h"
#include "core/editing/Editor.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/PlainTextRange.h"
#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/markers/DocumentMarkerController.h"
#include "core/editing/markers/SpellCheckMarker.h"
#include "core/editing/markers/SuggestionMarker.h"
#include "core/editing/markers/SuggestionMarkerReplacementScope.h"
#include "core/editing/spellcheck/SpellChecker.h"
#include "core/editing/suggestion/TextSuggestionInfo.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/layout/LayoutTheme.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

namespace {

bool ShouldDeleteNextCharacter(const Node& marker_text_node,
                               const DocumentMarker& marker) {
  // If the character immediately following the range to be deleted is a space,
  // delete it if either of these conditions holds:
  // - We're deleting at the beginning of the editable text (to avoid ending up
  //   with a space at the beginning)
  // - The character immediately before the range being deleted is also a space
  //   (to avoid ending up with two adjacent spaces)
  const EphemeralRange next_character_range =
      PlainTextRange(marker.EndOffset(), marker.EndOffset() + 1)
          .CreateRange(*marker_text_node.parentNode());
  // No character immediately following the range (so it can't be a space)
  if (next_character_range.IsNull())
    return false;

  const String next_character_str =
      PlainText(next_character_range, TextIteratorBehavior::Builder().Build());
  const UChar next_character = next_character_str[0];
  // Character immediately following the range is not a space
  if (next_character != kSpaceCharacter &&
      next_character != kNoBreakSpaceCharacter)
    return false;

  // First case: we're deleting at the beginning of the editable text
  if (marker.StartOffset() == 0)
    return true;

  const EphemeralRange prev_character_range =
      PlainTextRange(marker.StartOffset() - 1, marker.StartOffset())
          .CreateRange(*marker_text_node.parentNode());
  // Not at beginning, but there's no character immediately before the range
  // being deleted (so it can't be a space)
  if (prev_character_range.IsNull())
    return false;

  const String prev_character_str =
      PlainText(prev_character_range, TextIteratorBehavior::Builder().Build());
  // Return true if the character immediately before the range is a space, false
  // otherwise
  const UChar prev_character = prev_character_str[0];
  return prev_character == kSpaceCharacter ||
         prev_character == kNoBreakSpaceCharacter;
}

EphemeralRangeInFlatTree ComputeRangeSurroundingCaret(
    const PositionInFlatTree& caret_position) {
  const Node* const position_node = caret_position.ComputeContainerNode();
  const bool is_text_node = position_node->IsTextNode();
  const int position_offset_in_node =
      caret_position.ComputeOffsetInContainerNode();

  // If we're in the interior of a text node, we can avoid calling
  // PreviousPositionOf/NextPositionOf for better efficiency.
  if (is_text_node && position_offset_in_node != 0 &&
      position_offset_in_node != position_node->MaxCharacterOffset()) {
    return EphemeralRangeInFlatTree(
        PositionInFlatTree(position_node, position_offset_in_node - 1),
        PositionInFlatTree(position_node, position_offset_in_node + 1));
  }

  const PositionInFlatTree& previous_position =
      PreviousPositionOf(caret_position, PositionMoveType::kGraphemeCluster);

  const PositionInFlatTree& next_position =
      NextPositionOf(caret_position, PositionMoveType::kGraphemeCluster);

  return EphemeralRangeInFlatTree(
      previous_position.IsNull() ? caret_position : previous_position,
      next_position.IsNull() ? caret_position : next_position);
}

struct SuggestionInfosWithNodeAndHighlightColor {
  STACK_ALLOCATED();

  Persistent<Node> text_node;
  Color highlight_color;
  Vector<TextSuggestionInfo> suggestion_infos;
};

SuggestionInfosWithNodeAndHighlightColor ComputeSuggestionInfos(
    const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
        node_suggestion_marker_pairs,
    size_t max_number_of_suggestions) {
  // We look at all suggestion markers touching or overlapping the touched
  // location to pull suggestions from. We preferentially draw suggestions from
  // shorter markers first (since we assume they're more specific to the tapped
  // location) until we hit our limit.
  HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>
      node_suggestion_marker_pairs_sorted_by_length =
          node_suggestion_marker_pairs;
  std::sort(node_suggestion_marker_pairs_sorted_by_length.begin(),
            node_suggestion_marker_pairs_sorted_by_length.end(),
            [](const std::pair<Node*, DocumentMarker*>& pair1,
               const std::pair<Node*, DocumentMarker*>& pair2) {
              const int length1 =
                  pair1.second->EndOffset() - pair1.second->StartOffset();
              const int length2 =
                  pair2.second->EndOffset() - pair2.second->StartOffset();
              return length1 < length2;
            });

  SuggestionInfosWithNodeAndHighlightColor
      suggestion_infos_with_node_and_highlight_color;
  // In theory, a user could tap right before/after the start of a node and we'd
  // want to pull in suggestions from either side of the tap. However, this is
  // an edge case that's unlikely to matter in practice (the user will most
  // likely just tap in the node where they want to apply the suggestions) and
  // it complicates implementation, so we require that all suggestions come
  // from the same text node.
  suggestion_infos_with_node_and_highlight_color.text_node =
      node_suggestion_marker_pairs_sorted_by_length.front().first;

  // The highlight color comes from the shortest suggestion marker touching or
  // intersecting the tapped location. If there's no color set, we use the
  // default text selection color.
  const SuggestionMarker& first_suggestion_marker = *ToSuggestionMarker(
      node_suggestion_marker_pairs_sorted_by_length.front().second);

  suggestion_infos_with_node_and_highlight_color.highlight_color =
      (first_suggestion_marker.SuggestionHighlightColor() == 0)
          ? LayoutTheme::TapHighlightColor()
          : first_suggestion_marker.SuggestionHighlightColor();

  Vector<TextSuggestionInfo>& suggestion_infos =
      suggestion_infos_with_node_and_highlight_color.suggestion_infos;
  for (const std::pair<Node*, DocumentMarker*>& node_marker_pair :
       node_suggestion_marker_pairs_sorted_by_length) {
    if (node_marker_pair.first !=
        suggestion_infos_with_node_and_highlight_color.text_node)
      continue;

    if (suggestion_infos.size() == max_number_of_suggestions)
      break;

    const SuggestionMarker* marker =
        ToSuggestionMarker(node_marker_pair.second);
    const Vector<String>& marker_suggestions = marker->Suggestions();
    for (size_t suggestion_index = 0;
         suggestion_index < marker_suggestions.size(); ++suggestion_index) {
      const String& suggestion = marker_suggestions[suggestion_index];
      if (suggestion_infos.size() == max_number_of_suggestions)
        break;
      if (std::find_if(suggestion_infos.begin(), suggestion_infos.end(),
                       [marker, &suggestion](const TextSuggestionInfo& info) {
                         return info.span_start ==
                                    (int32_t)marker->StartOffset() &&
                                info.span_end == (int32_t)marker->EndOffset() &&
                                info.suggestion == suggestion;
                       }) != suggestion_infos.end())
        continue;

      TextSuggestionInfo suggestion_info;
      suggestion_info.marker_tag = marker->Tag();
      suggestion_info.suggestion_index = suggestion_index;
      suggestion_info.span_start = marker->StartOffset();
      suggestion_info.span_end = marker->EndOffset();
      suggestion_info.suggestion = suggestion;
      suggestion_infos.push_back(suggestion_info);
    }
  }

  return suggestion_infos_with_node_and_highlight_color;
}

}  // namespace

TextSuggestionController::TextSuggestionController(LocalFrame& frame)
    : is_suggestion_menu_open_(false), frame_(&frame) {}

void TextSuggestionController::DocumentAttached(Document* document) {
  DCHECK(document);
  SetContext(document);
}

bool TextSuggestionController::IsMenuOpen() const {
  return is_suggestion_menu_open_;
}

void TextSuggestionController::HandlePotentialSuggestionTap(
    const PositionInFlatTree& caret_position) {
  // TODO(crbug.com/779126): add support for suggestions in immersive mode.
  if (GetDocument().GetSettings()->GetImmersiveModeEnabled())
    return;

  // It's theoretically possible, but extremely unlikely, that the user has
  // managed to tap on some text after TextSuggestionController has told the
  // browser to open the text suggestions menu, but before the browser has
  // actually done so. In this case, we should just ignore the tap.
  if (is_suggestion_menu_open_)
    return;

  const EphemeralRangeInFlatTree& range_to_check =
      ComputeRangeSurroundingCaret(caret_position);

  const std::pair<const Node*, const DocumentMarker*>& node_and_marker =
      FirstMarkerIntersectingRange(
          range_to_check, DocumentMarker::kSpelling | DocumentMarker::kGrammar |
                              DocumentMarker::kSuggestion);
  if (!node_and_marker.first)
    return;

  if (!text_suggestion_host_) {
    GetFrame().GetInterfaceProvider().GetInterface(
        mojo::MakeRequest(&text_suggestion_host_));
  }

  text_suggestion_host_->StartSuggestionMenuTimer();
}

void TextSuggestionController::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  DocumentShutdownObserver::Trace(visitor);
}

void TextSuggestionController::ReplaceActiveSuggestionRange(
    const String& suggestion) {
  const VisibleSelectionInFlatTree& selection =
      GetFrame().Selection().ComputeVisibleSelectionInFlatTree();
  if (selection.IsNone())
    return;

  const EphemeralRangeInFlatTree& range_to_check =
      selection.IsRange() ? selection.ToNormalizedEphemeralRange()
                          : ComputeRangeSurroundingCaret(selection.Start());
  const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
      node_marker_pairs =
          GetFrame().GetDocument()->Markers().MarkersIntersectingRange(
              range_to_check, DocumentMarker::kActiveSuggestion);

  if (node_marker_pairs.IsEmpty())
    return;

  Node* const marker_text_node = node_marker_pairs.front().first;
  const DocumentMarker* const marker = node_marker_pairs.front().second;

  const EphemeralRange& range_to_replace =
      EphemeralRange(Position(marker_text_node, marker->StartOffset()),
                     Position(marker_text_node, marker->EndOffset()));
  ReplaceRangeWithText(range_to_replace, suggestion);
}

void TextSuggestionController::ApplySpellCheckSuggestion(
    const String& suggestion) {
  ReplaceActiveSuggestionRange(suggestion);
  OnSuggestionMenuClosed();
}

void TextSuggestionController::ApplyTextSuggestion(int32_t marker_tag,
                                                   uint32_t suggestion_index) {
  const VisibleSelectionInFlatTree& selection =
      GetFrame().Selection().ComputeVisibleSelectionInFlatTree();
  if (selection.IsNone()) {
    OnSuggestionMenuClosed();
    return;
  }

  const EphemeralRangeInFlatTree& range_to_check =
      selection.IsRange() ? selection.ToNormalizedEphemeralRange()
                          : ComputeRangeSurroundingCaret(selection.Start());

  const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
      node_marker_pairs =
          GetFrame().GetDocument()->Markers().MarkersIntersectingRange(
              range_to_check, DocumentMarker::kSuggestion);

  const Node* marker_text_node = nullptr;
  SuggestionMarker* marker = nullptr;
  for (const std::pair<Member<Node>, Member<DocumentMarker>>& node_marker_pair :
       node_marker_pairs) {
    SuggestionMarker* suggestion_marker =
        ToSuggestionMarker(node_marker_pair.second);
    if (suggestion_marker->Tag() == marker_tag) {
      marker_text_node = node_marker_pair.first;
      marker = suggestion_marker;
      break;
    }
  }

  if (!marker) {
    OnSuggestionMenuClosed();
    return;
  }

  const EphemeralRange& range_to_replace =
      EphemeralRange(Position(marker_text_node, marker->StartOffset()),
                     Position(marker_text_node, marker->EndOffset()));

  const String& replacement = marker->Suggestions()[suggestion_index];
  const String& new_suggestion = PlainText(range_to_replace);

  {
    SuggestionMarkerReplacementScope scope;
    ReplaceRangeWithText(range_to_replace, replacement);
  }

  if (marker->IsMisspelling()) {
    GetFrame().GetDocument()->Markers().RemoveSuggestionMarkerByTag(
        marker_text_node, marker->Tag());
  } else {
    marker->SetSuggestion(suggestion_index, new_suggestion);
  }

  OnSuggestionMenuClosed();
}

void TextSuggestionController::DeleteActiveSuggestionRange() {
  AttemptToDeleteActiveSuggestionRange();
  OnSuggestionMenuClosed();
}

void TextSuggestionController::OnNewWordAddedToDictionary(const String& word) {
  // Android pops up a dialog to let the user confirm they actually want to add
  // the word to the dictionary; this method gets called as soon as the dialog
  // is shown. So the word isn't actually in the dictionary here, even if the
  // user will end up confirming the dialog, and we shouldn't try to re-run
  // spellcheck here.

  // Note: this actually matches the behavior in native Android text boxes
  GetDocument().Markers().RemoveSpellingMarkersUnderWords(
      Vector<String>({word}));
  OnSuggestionMenuClosed();
}

void TextSuggestionController::OnSuggestionMenuClosed() {
  if (!IsAvailable())
    return;

  GetDocument().Markers().RemoveMarkersOfTypes(
      DocumentMarker::kActiveSuggestion);
  GetFrame().Selection().SetCaretVisible(true);
  is_suggestion_menu_open_ = false;
}

void TextSuggestionController::SuggestionMenuTimeoutCallback(
    size_t max_number_of_suggestions) {
  if (!IsAvailable())
    return;

  const VisibleSelectionInFlatTree& selection =
      GetFrame().Selection().ComputeVisibleSelectionInFlatTree();
  if (selection.IsNone())
    return;

  const EphemeralRangeInFlatTree& range_to_check =
      selection.IsRange() ? selection.ToNormalizedEphemeralRange()
                          : ComputeRangeSurroundingCaret(selection.Start());

  // We can show a menu if the user tapped on either a spellcheck marker or a
  // suggestion marker. Suggestion markers take precedence (we don't even try
  // to draw both underlines, suggestion wins).
  const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
      node_suggestion_marker_pairs =
          GetFrame().GetDocument()->Markers().MarkersIntersectingRange(
              range_to_check, DocumentMarker::kSuggestion);
  if (!node_suggestion_marker_pairs.IsEmpty()) {
    ShowSuggestionMenu(node_suggestion_marker_pairs, max_number_of_suggestions);
    return;
  }

  // If we didn't find any suggestion markers, look for spell check markers.
  const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>
      node_spelling_marker_pairs =
          GetFrame().GetDocument()->Markers().MarkersIntersectingRange(
              range_to_check, DocumentMarker::MisspellingMarkers());
  if (!node_spelling_marker_pairs.IsEmpty())
    ShowSpellCheckMenu(node_spelling_marker_pairs.front());

  // If we get here, that means the user tapped on a spellcheck or suggestion
  // marker a few hundred milliseconds ago (to start the double-click timer)
  // but it's gone now. Oh well...
}

void TextSuggestionController::ShowSpellCheckMenu(
    const std::pair<Node*, DocumentMarker*>& node_spelling_marker_pair) {
  Node* const marker_text_node = node_spelling_marker_pair.first;
  SpellCheckMarker* const marker =
      ToSpellCheckMarker(node_spelling_marker_pair.second);

  const EphemeralRange active_suggestion_range =
      EphemeralRange(Position(marker_text_node, marker->StartOffset()),
                     Position(marker_text_node, marker->EndOffset()));
  const String& misspelled_word = PlainText(active_suggestion_range);
  const String& description = marker->Description();

  is_suggestion_menu_open_ = true;
  GetFrame().Selection().SetCaretVisible(false);
  GetDocument().Markers().AddActiveSuggestionMarker(
      active_suggestion_range, SK_ColorTRANSPARENT,
      StyleableMarker::Thickness::kThin,
      LayoutTheme::GetTheme().PlatformActiveSpellingMarkerHighlightColor());

  Vector<String> suggestions;
  description.Split('\n', suggestions);

  Vector<mojom::blink::SpellCheckSuggestionPtr> suggestion_ptrs;
  for (const String& suggestion : suggestions) {
    mojom::blink::SpellCheckSuggestionPtr info_ptr(
        mojom::blink::SpellCheckSuggestion::New());
    info_ptr->suggestion = suggestion;
    suggestion_ptrs.push_back(std::move(info_ptr));
  }

  const IntRect& absolute_bounds = GetFrame().Selection().AbsoluteCaretBounds();
  const IntRect& viewport_bounds =
      GetFrame().View()->ContentsToViewport(absolute_bounds);

  text_suggestion_host_->ShowSpellCheckSuggestionMenu(
      viewport_bounds.X(), viewport_bounds.MaxY(), std::move(misspelled_word),
      std::move(suggestion_ptrs));
}

void TextSuggestionController::ShowSuggestionMenu(
    const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
        node_suggestion_marker_pairs,
    size_t max_number_of_suggestions) {
  DCHECK(!node_suggestion_marker_pairs.IsEmpty());

  SuggestionInfosWithNodeAndHighlightColor
      suggestion_infos_with_node_and_highlight_color = ComputeSuggestionInfos(
          node_suggestion_marker_pairs, max_number_of_suggestions);

  Vector<TextSuggestionInfo>& suggestion_infos =
      suggestion_infos_with_node_and_highlight_color.suggestion_infos;
  int span_union_start = suggestion_infos[0].span_start;
  int span_union_end = suggestion_infos[0].span_end;
  for (size_t i = 1; i < suggestion_infos.size(); ++i) {
    span_union_start =
        std::min(span_union_start, suggestion_infos[i].span_start);
    span_union_end = std::max(span_union_end, suggestion_infos[i].span_end);
  }

  const Node* text_node =
      suggestion_infos_with_node_and_highlight_color.text_node;
  for (TextSuggestionInfo& info : suggestion_infos) {
    const EphemeralRange prefix_range(Position(text_node, span_union_start),
                                      Position(text_node, info.span_start));
    const String& prefix = PlainText(prefix_range);

    const EphemeralRange suffix_range(Position(text_node, info.span_end),
                                      Position(text_node, span_union_end));
    const String& suffix = PlainText(suffix_range);

    info.prefix = prefix;
    info.suffix = suffix;
  }

  const EphemeralRange marker_range(Position(text_node, span_union_start),
                                    Position(text_node, span_union_end));

  GetDocument().Markers().AddActiveSuggestionMarker(
      marker_range, SK_ColorTRANSPARENT, StyleableMarker::Thickness::kThin,
      suggestion_infos_with_node_and_highlight_color.highlight_color);

  is_suggestion_menu_open_ = true;
  GetFrame().Selection().SetCaretVisible(false);

  const String& misspelled_word = PlainText(marker_range);
  CallMojoShowTextSuggestionMenu(
      suggestion_infos_with_node_and_highlight_color.suggestion_infos,
      misspelled_word);
}

void TextSuggestionController::CallMojoShowTextSuggestionMenu(
    const Vector<TextSuggestionInfo>& text_suggestion_infos,
    const String& misspelled_word) {
  Vector<mojom::blink::TextSuggestionPtr> suggestion_info_ptrs;
  for (const blink::TextSuggestionInfo& info : text_suggestion_infos) {
    mojom::blink::TextSuggestionPtr info_ptr(
        mojom::blink::TextSuggestion::New());
    info_ptr->marker_tag = info.marker_tag;
    info_ptr->suggestion_index = info.suggestion_index;
    info_ptr->prefix = info.prefix;
    info_ptr->suggestion = info.suggestion;
    info_ptr->suffix = info.suffix;

    suggestion_info_ptrs.push_back(std::move(info_ptr));
  }

  const IntRect& absolute_bounds = GetFrame().Selection().AbsoluteCaretBounds();
  const IntRect& viewport_bounds =
      GetFrame().View()->ContentsToViewport(absolute_bounds);

  text_suggestion_host_->ShowTextSuggestionMenu(
      viewport_bounds.X(), viewport_bounds.MaxY(), misspelled_word,
      std::move(suggestion_info_ptrs));
}

Document& TextSuggestionController::GetDocument() const {
  DCHECK(IsAvailable());
  return *LifecycleContext();
}

bool TextSuggestionController::IsAvailable() const {
  return LifecycleContext();
}

LocalFrame& TextSuggestionController::GetFrame() const {
  DCHECK(frame_);
  return *frame_;
}

std::pair<const Node*, const DocumentMarker*>
TextSuggestionController::FirstMarkerIntersectingRange(
    const EphemeralRangeInFlatTree& range,
    DocumentMarker::MarkerTypes types) const {
  const Node* const range_start_container =
      range.StartPosition().ComputeContainerNode();
  const int range_start_offset =
      range.StartPosition().ComputeOffsetInContainerNode();
  const Node* const range_end_container =
      range.EndPosition().ComputeContainerNode();
  const int range_end_offset =
      range.EndPosition().ComputeOffsetInContainerNode();

  for (const Node& node : range.Nodes()) {
    if (!node.IsTextNode())
      continue;

    const int start_offset =
        node == range_start_container ? range_start_offset : 0;
    const int end_offset = node == range_end_container
                               ? range_end_offset
                               : node.MaxCharacterOffset();

    const DocumentMarker* const found_marker =
        GetFrame().GetDocument()->Markers().FirstMarkerIntersectingOffsetRange(
            ToText(node), start_offset, end_offset, types);
    if (found_marker)
      return std::make_pair(&node, found_marker);
  }

  return {};
}

std::pair<const Node*, const DocumentMarker*>
TextSuggestionController::FirstMarkerTouchingSelection(
    DocumentMarker::MarkerTypes types) const {
  const VisibleSelectionInFlatTree& selection =
      GetFrame().Selection().ComputeVisibleSelectionInFlatTree();
  if (selection.IsNone())
    return {};

  const EphemeralRangeInFlatTree& range_to_check =
      selection.IsRange()
          ? EphemeralRangeInFlatTree(selection.Start(), selection.End())
          : ComputeRangeSurroundingCaret(selection.Start());

  return FirstMarkerIntersectingRange(range_to_check, types);
}

void TextSuggestionController::AttemptToDeleteActiveSuggestionRange() {
  const std::pair<const Node*, const DocumentMarker*>& node_and_marker =
      FirstMarkerTouchingSelection(DocumentMarker::kActiveSuggestion);
  if (!node_and_marker.first)
    return;

  const Node* const marker_text_node = node_and_marker.first;
  const DocumentMarker* const marker = node_and_marker.second;

  const bool delete_next_char =
      ShouldDeleteNextCharacter(*marker_text_node, *marker);

  const EphemeralRange range_to_delete = EphemeralRange(
      Position(marker_text_node, marker->StartOffset()),
      Position(marker_text_node, marker->EndOffset() + delete_next_char));
  ReplaceRangeWithText(range_to_delete, "");
}

void TextSuggestionController::ReplaceRangeWithText(const EphemeralRange& range,
                                                    const String& replacement) {
  GetFrame().Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder().SetBaseAndExtent(range).Build());

  // TODO(editing-dev): We should check whether |TextSuggestionController| is
  // available or not.
  // TODO(editing-dev): The use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  See http://crbug.com/590369 for more details.
  GetFrame().GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  // Dispatch 'beforeinput'.
  Element* const target = FindEventTargetFrom(
      GetFrame(), GetFrame().Selection().ComputeVisibleSelectionInDOMTree());

  DataTransfer* const data_transfer = DataTransfer::Create(
      DataTransfer::DataTransferType::kInsertReplacementText,
      DataTransferAccessPolicy::kDataTransferReadable,
      DataObject::CreateFromString(replacement));

  const bool is_canceled =
      DispatchBeforeInputDataTransfer(
          target, InputEvent::InputType::kInsertReplacementText,
          data_transfer) != DispatchEventResult::kNotCanceled;

  // 'beforeinput' event handler may destroy target frame.
  if (!IsAvailable())
    return;

  // TODO(editing-dev): The use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  See http://crbug.com/590369 for more details.
  GetFrame().GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  if (is_canceled)
    return;
  GetFrame().GetEditor().ReplaceSelectionWithText(
      replacement, false, false, InputEvent::InputType::kInsertReplacementText);
}

}  // namespace blink
