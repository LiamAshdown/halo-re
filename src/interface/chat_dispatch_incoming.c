// chat_dispatch_incoming  (Ghidra: FUN_004aaf70, renamed)
// address 0x4aaf70, size 502 bytes
// name confidence: 0.45 (chosen)   rewrite confidence: 0.6
// evidence: same validate/reject shape as the game_engine_apply_*_message handlers in src/game/
// (message_delta_decode_compound_field decodes EAX event into the ECX record, message_delta_decode_compound_field_staged is the rejection path); the
// decoded record is turned into a line for chimera__multiplayer_message @0x4ab4b0.
// Rewritten in the phase-4 review from objdump -d 0x4aaf70..0x4ab166: Ghidra removed 21 blocks
// as unreachable (it lost the AL result of message_delta_decode_compound_field), so its body below is only the gate.
// Frame: record at esp+0x0c {int32 kind, uint8 player (0xff none), pad, wchar_t *text}, text
// buffer 0x100 wide chars at +0x518, line buffer 0x200 wide chars at +0x118, short line 0x80 wide
// chars at +0x18.
// Kinds: 0 player message, string 0xbb of ui\multiplayer_game_text as the prefix format;
// 1 and 2 team / vehicle message, string 0xbc; any other kind with a player prints the raw text;
// kind 4 without a player is a localized string id (decimal text, _wtol) loaded with
// shell_load_localized_string and widened through L"%S"; kind 3 without a player prints the
// text as is.
// UNSURE: message_delta_decode_compound_field/message_delta_decode_compound_field_staged identity is inherited from src/game/; the length computed by
// wcslen after the prefix format is discarded in the binary as well.
// register convention: event in EAX.
//   // blam-cc: event -> EAX

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;          // 0x0087a480
extern void *shell_module_handle;        // 0x00722bb8, UNSURE: passed in ECX to the string loader
extern wchar_t empty_string;             // 0x00660c34

extern uint8_t message_delta_decode_compound_field(void *event, chat_incoming_record *out_record); // 0x4ec590, blam-cc: EAX event, ECX out
extern void message_delta_decode_compound_field_staged(void *event);    // 0x4ec670, blam-cc: EAX event
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern wchar_t *text_string_list_get_string(datum_index tag, int16_t index); // 0x5578c0, blam-cc: ECX tag, DX index
extern wchar_t *string_format_wide_va(wchar_t *dest, const wchar_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
extern int32_t shell_load_localized_string(int32_t id, char *out_buffer); // 0x57e1a0, blam-cc: ECX module, EAX size 0x200, ESI out
extern void chimera__multiplayer_message(const wchar_t *text); // 0x4ab4b0

static const wchar_t *chat_prefix_format(int16_t string_index)
{
    datum_index tag = tag_lookup(0x75737472 /* ustr */, (char *)"ui\\multiplayer_game_text"); // 'ustr'
    if (tag == (datum_index)-1) {
        return &empty_string;
    }
    return text_string_list_get_string(tag, string_index);
}

// blam-cc: event -> EAX
// Decodes one incoming chat network event and appends the resulting line to the chat listbox.
void chat_dispatch_incoming(void *event)
{
    chat_incoming_record record;
    wchar_t short_line[0x80];
    wchar_t line[0x200];
    wchar_t text[0x100];

    if (*(int32_t *)*(void **)event != 0) {
        message_delta_decode_compound_field_staged(event);
        return;
    }

    record.kind = 0;
    record.player_index = 0xff;
    record.text = (uint16_t *)text;
    if (!message_delta_decode_compound_field(event, &record)) {
        return;
    }

    if (record.player_index != 0xff) {
        player *sender = (player *)datum_get((datum_index)record.player_index, player_data);
        if (sender == 0) {
            return;
        }
        memset(line, 0, sizeof(line));
        if (record.kind == 0) {
            string_format_wide_va(line, chat_prefix_format(0xbb), sender->name);
            wcslen(line);
        } else if (record.kind > 0 && record.kind <= 2) {
            string_format_wide_va(line, chat_prefix_format(0xbc), sender->name);
            wcslen(line);
        }
        wcscat(line, text);
        chimera__multiplayer_message(line);
        return;
    }

    if (record.kind == 4) {
        char localized[0x400];  // the buffer at +0x118 reused as 8-bit text
        int32_t string_id = (int32_t)_wtol((const wchar_t *)record.text);
        memset(short_line, 0, sizeof(short_line));
        if (shell_load_localized_string(string_id, localized) == 0) {
            return;
        }
        string_format_wide_va_bounded(0x7f, (uint16_t *)short_line, (const uint16_t *)L"%S", localized); // EDX 0x7f
        short_line[0x7f] = 0;
        chimera__multiplayer_message(short_line);
        return;
    }

    if (record.kind == 3) {
        chimera__multiplayer_message(text);
    }
}

#if 0
Original Ghidra decompilation (0x4aaf70):

/* WARNING: Removing unreachable block (ram,0x004aafbe) */
/* WARNING: Removing unreachable block (ram,0x004aafd6) */
/* WARNING: Removing unreachable block (ram,0x004aafec) */
/* WARNING: Removing unreachable block (ram,0x004aafee) */
/* WARNING: Removing unreachable block (ram,0x004ab04c) */
/* WARNING: Removing unreachable block (ram,0x004ab071) */
/* WARNING: Removing unreachable block (ram,0x004ab063) */
/* WARNING: Removing unreachable block (ram,0x004ab076) */
/* WARNING: Removing unreachable block (ram,0x004ab143) */
/* WARNING: Removing unreachable block (ram,0x004ab0cc) */
/* WARNING: Removing unreachable block (ram,0x004ab0e8) */
/* WARNING: Removing unreachable block (ram,0x004ab0ea) */
/* WARNING: Removing unreachable block (ram,0x004ab10b) */
/* WARNING: Removing unreachable block (ram,0x004aaff8) */
/* WARNING: Removing unreachable block (ram,0x004aaffe) */
/* WARNING: Removing unreachable block (ram,0x004ab007) */
/* WARNING: Removing unreachable block (ram,0x004ab02c) */
/* WARNING: Removing unreachable block (ram,0x004ab01e) */
/* WARNING: Removing unreachable block (ram,0x004ab031) */
/* WARNING: Removing unreachable block (ram,0x004ab08f) */
/* WARNING: Removing unreachable block (ram,0x004ab097) */

void FUN_004aaf70(void)

{
  undefined4 *in_EAX;

  if (*(int *)*in_EAX == 0) {
    FUN_004ec590();
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
