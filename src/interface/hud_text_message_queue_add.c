// hud_text_message_queue_add  (Ghidra: hud_text_message_queue_add, already named)
// address 0x4a3d90, size 145 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x4a3d90..0x4a3e20 (EAX text, EBX top, stack index; \s<n> spacer lines and \h hold).)
// evidence: matches the given name; functions.md: "Appends a HUD text/chat message to the message
// queue, recognizing embedded \s (sound) and \h escape markers." types/interface.h's
// hud_text_message struct (text/unknown_04/hold/start_time/end_time) matches this function's
// field writes exactly, confirming its own "growable_array hud_text_message_queue" doc.
// register convention: text in EAX (in_EAX), start_time in EBX (unaff_EBX), tag/index as the one
// recognized stack parameter. // blam-cc: EAX -> text, EBX -> start_time, stack -> tag
// UNSURE: DAT_00660c34 is a shared "blank" wide-string constant referenced by several already-
// rewritten files' wcslen (wide strlen) calls; reused here with the same name.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern growable_array hud_text_message_queue; // 0x006b37e8
extern uint16_t empty_string[];  // 0x00660c34

extern int32_t growable_array_add_element(growable_array *array); // 0x4cf810

// blam-cc: EAX -> text, EBX -> start_time, stack -> tag
// Appends `text` to the HUD text-message queue with `start_time` as its arrival time, recognizing
// a leading "\sNNNN" (a sound-delay count in units of 16ms, replacing the text with a blank
// placeholder) or "\h" (marks the message as "hold", skipping past the marker) escape. Returns the
// message's duration in milliseconds (or 0 if `text` is empty).
int32_t hud_text_message_queue_add(uint16_t *text, int32_t start_time, int32_t tag)
{
    hud_text_message *message;
    uint16_t *body;
    int32_t index;

    if (wcslen(text) == 0) {
        return 0;
    }

    index = growable_array_add_element(&hud_text_message_queue);
    message = (hud_text_message *)hud_text_message_queue.data + index;
    message->start_time = start_time;
    message->tag = tag;
    message->hold = 0;

    body = text;
    if (text[0] == '\\') {
        body = text + 1;
        if (text[1] == 's') {
            int32_t delay = _wtol(text + 2);

            message->text = empty_string;
            message->end_time = delay * 0x10 + start_time;
            return delay * 0x10;
        }
        if (text[1] == 'h') {
            body = text + 2;
            message->hold = 1;
        }
    }
    message->text = body;
    message->end_time = start_time + 0x10;
    return 0x10;
}

#if 0
Original Ghidra decompilation (0x4a3d90):

int hud_text_message_queue_add(undefined4 param_1)

{
  undefined4 *puVar1;
  short *in_EAX;
  int iVar2;
  long lVar3;
  int unaff_EBX;
  short *psVar4;

  iVar2 = FUN_00625b7a();
  if (iVar2 == 0) {
    return 0;
  }
  iVar2 = growable_array_add_element();
  puVar1 = (undefined4 *)(DAT_006b37f0 + iVar2 * 0x14);
  puVar1[3] = unaff_EBX;
  puVar1[1] = param_1;
  puVar1[2] = 0;
  psVar4 = in_EAX;
  if (*in_EAX == 0x5c) {
    psVar4 = in_EAX + 1;
    if (in_EAX[1] == 0x73) {
      lVar3 = __wtol(in_EAX + 2);
      *puVar1 = &DAT_00660c34;
      puVar1[4] = lVar3 * 0x10 + unaff_EBX;
      return lVar3 * 0x10;
    }
    if (in_EAX[1] == 0x68) {
      psVar4 = in_EAX + 2;
      puVar1[2] = 1;
    }
  }
  *puVar1 = psVar4;
  puVar1[4] = unaff_EBX + 0x10;
  return 0x10;
}
#endif
