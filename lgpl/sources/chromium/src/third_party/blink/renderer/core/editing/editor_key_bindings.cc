/*
 * Copyright (C) 2006, 2007 Apple, Inc.  All rights reserved.
 * Copyright (C) 2012 Google, Inc.  All rights reserved.
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

#include "third_party/blink/renderer/core/editing/editor.h"

#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/renderer/core/editing/commands/editor_command.h"
#include "third_party/blink/renderer/core/editing/editing_behavior.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/events/keyboard_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/platform/keyboard_codes.h"

namespace blink {

bool Editor::HandleEditingKeyboardEvent(KeyboardEvent* evt) {
  const WebKeyboardEvent* key_event = evt->KeyEvent();
  if (!key_event)
    return false;
  // do not treat this as text input if it's a system key event
  bool is_system_key = key_event->is_system_key;
#if defined(OS_WIN)
  // Do not treat Alt[+Shift]+Backspace as a system key to make it an Undo/Redo
  // command
  if ((key_event->GetModifiers() & WebInputEvent::kAltKey) &&
      key_event->windows_key_code == blink::VKEY_BACK)
    is_system_key = false;
#endif  // OS_WIN
  if (is_system_key)
    return false;

  String command_name = Behavior().InterpretKeyEvent(*evt);
  const EditorCommand command = this->CreateCommand(command_name);

  if (key_event->GetType() == WebInputEvent::kRawKeyDown) {
    // WebKit doesn't have enough information about mode to decide how
    // commands that just insert text if executed via Editor should be treated,
    // so we leave it upon WebCore to either handle them immediately
    // (e.g. Tab that changes focus) or let a keypress event be generated
    // (e.g. Tab that inserts a Tab character, or Enter).
    if (command.IsTextInsertion() || command_name.IsEmpty())
      return false;
    return command.Execute(evt);
  }

  if (command.Execute(evt))
    return true;

  if (!Behavior().ShouldInsertCharacter(*evt) || !CanEdit())
    return false;

  const Element* const focused_element =
      frame_->GetDocument()->FocusedElement();
  if (!focused_element) {
    // We may lose focused element by |command.execute(evt)|.
    return false;
  }
  // We should not insert text at selection start if selection doesn't have
  // focus.
  if (!frame_->Selection().SelectionHasFocus())
    return false;

  // Return true to prevent default action. e.g. Space key scroll.
  if (DispatchBeforeInputInsertText(evt->target()->ToNode(), key_event->text) !=
      DispatchEventResult::kNotCanceled)
    return true;

  return InsertText(key_event->text, evt);
}

void Editor::HandleKeyboardEvent(KeyboardEvent* evt) {
  // Give the embedder a chance to handle the keyboard event.
  if (frame_->Client()->HandleCurrentKeyboardEvent() ||
      HandleEditingKeyboardEvent(evt)) {
    evt->SetDefaultHandled();
  }
}

}  // namespace blink