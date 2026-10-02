// game_engine_build_kill_feed_message_text  (Ghidra: game_engine_build_kill_feed_message_text,
// already named)
// address 0x45e680, size 2815 bytes
// name confidence: 0.55   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Formats the localized text for a specific
// kill-feed/HUD message type (kill, suicide, betrayal, etc.) into a caller-supplied buffer");
// game_engine_on_player_death.c's two call sites (message types 0x0d and 8, buffer size 0x400),
// which is what pins the parameter order/meaning used here.
// register convention: output buffer in EBX (unaff_EBX); message_type, subject (param_2) and
// buffer_size are this function's own stack parameters, kept in original order after EBX.
//   // blam-cc: EBX -> out, stack -> message_type, subject, buffer_size
// UNSURE (the whole switch): Ghidra could not recover the jump table at 0x45f198 as a normal
// switch and instead printed each arm's *destination address* as the case label. The table was
// recovered by hand from the raw bytes (objdump -s -j .text --start-address=0x45f198
// --stop-address=0x45f218 bin/halo.exe) and is reproduced below as `switch (adjusted_type)` with
// case values equal to the table's own index (0..0x1f) -- this part is HIGH confidence, since it
// is a direct transcription of 32 consecutive dwords, not a guess. What is NOT recoverable from
// this decompilation is almost every individual call's arguments: `datum_get()`,
// `text_string_list_get_string()` and `string_format_wide_va_bounded()` are shown with zero
// visible operands at dozens of call sites (the registers carrying them are never named because
// Ghidra's jump-table failure also broke its register tracking through the whole function).
// Each such call below is modeled with the most plausible argument from context (usually
// `subject`, the one player-like handle this function receives) and flagged individually;
// treat the specific %s/%d fill-ins as unverified. The *branching structure itself* -- which
// case reaches which fallback label, and in what order calls happen -- is transcribed exactly.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern wchar_t empty_string;                          // 0x00660c34

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest);
    // 0x466530, this module; blam-cc: ECX -> ticks, stack -> (unused, dest)
extern char input_get_last_used_binding(void *out_140_bytes); // 0x48bde0, not in this batch
extern void input_get_binding_display_name(void); // 0x48c7f0, not in this batch
// wcslen is 0x625b7a (identified from objdump: it walks 16-bit units, divides the byte
// distance by 2 and subtracts 1). Declared by <wchar.h>, so no extern is written for it.

// blam-cc: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size
// (EAX, copied to EDI at 0x45e697, is the player handle the text is for: the same value the callers hand the engine's
//  +0x6c override as its first argument. The body below was written without it and does not use it yet.)
// Formats one of the engine's localized kill-feed message strings into `out` (bounded to
// `buffer_size` wchar_t's), selecting among ~30 message types via the game engine's jump table,
// with a handful of them (7..12) remapped to a "team-aware" phrasing variant while a team game
// is active and the engine opts in via its unknown_84 callback. Returns true if a message was
// built, false for an out-of-range/unhandled type.
uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size)
{
    uint32_t adjusted_type = message_type;
    uint8_t ok = 1;

    if (current_game_engine != 0 && current_game_engine->unknown_84 != 0 &&
        ((char (*)(int32_t))current_game_engine->unknown_84)(1) != 0) {
        switch (message_type) {
            case 7: adjusted_type = 0x10; break;
            case 8: adjusted_type = 0x13; break;
            case 9: adjusted_type = 0x0f; break;
            case 10: adjusted_type = 0x0e; break;
            case 11: adjusted_type = 0x12; break;
            case 12: adjusted_type = 0x11; break;
            default: break;
        }
    }

    if (adjusted_type < 0x20) {
        switch (adjusted_type) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x06: case 0x0d: case 0x1c: {
            // T[0,1,2,3,6,13,28]: cases A/B/C/D/H/I/G -- single datum_get(subject) guard, then a
            // localized string formatted with `subject` as its one %s/%d argument.
            void *element = datum_get(subject, player_data); // UNSURE: array arg guessed as player_data
            if (element == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : text_string_list_get_string(tag_id, (int16_t)adjusted_type); // UNSURE index
                string_format_wide_va_bounded(out, fmt, subject); // UNSURE args
            }
            break;
        }
        case 0x04: case 0x05: {
            // T[4,5]: cases E/F -- two datum_get guards (both subject-shaped; the second
            // handle's true source is not recoverable from this decompilation).
            void *a = datum_get(subject, player_data);
            void *b = datum_get(subject, player_data); // UNSURE: second handle unknown
            if (a == 0 || b == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                string_format_wide_va_bounded(out, fmt, subject, subject); // UNSURE args
            }
            break;
        }
        case 0x07: case 0x09: case 0x0a: case 0x0b: case 0x0c: {
            // T[7,9,10,11,12]: cases L/K/J/N/M -- no datum_get guard; look up a plain string,
            // copy it verbatim, then queue an announcer sound.
            datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
            wchar_t *text = (tag_id == k_datum_index_none) ? (wchar_t *)L""
                : text_string_list_get_string(tag_id, (int16_t)adjusted_type); // UNSURE index
            wcsncpy(out, text, buffer_size);
            // FIXED 2026-09-28: this group queues no sound -- the only five calls to 0x46be40 in the
            //   function sit in the adjusted-type 0x0e..0x12 arms (0x45eced..0x45eeb7).
            break;
        }
        case 0x08: {
            // T[8]: case O -- datum_get guard, then a format with or without a looked-up prefix.
            void *element = datum_get(subject, player_data);
            if (element == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                if (tag_id == k_datum_index_none) {
                    string_format_wide_va_bounded(out, L"%d", subject); // UNSURE fmt/args
                } else {
                    wchar_t *fmt = text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                    string_format_wide_va_bounded(out, fmt, subject); // UNSURE args
                }
            }
            break;
        }
        case 0x0e: case 0x0f: case 0x10: case 0x11: case 0x12: {
            // T[14..18]: cases P/Q/R/S/T -- datum_get guard, then the engine's own get_score
            // callback, then a looked-up format string, then wcsncpy'd out like the L/K/J/N/M
            // group above (each shares that group's own success path in the original).
            void *element = datum_get(subject, player_data);
            if (element == 0) {
                ok = 0;
                break;
            }
            if (current_game_engine != 0 && current_game_engine->get_score != 0) {
                ((int32_t (*)(datum_index))current_game_engine->get_score)(subject); // UNSURE args
            }
            {
                datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                string_format_wide_va_bounded(out, fmt, subject); // UNSURE args
            }
            // FIXED 2026-09-28 from the arms at 0x45ec87/0x45ecfa/0x45ed6c/0x45eddf/0x45ee52 (jump table
            //   0x45f198 entries 0x0e..0x12): sounds 0x10, 0x0f, 0x0e, 0x11, 0x12, no player, no broadcast.
            {
                static const uint8_t arm_sound[5] = { 0x10, 0x0f, 0x0e, 0x11, 0x12 };

                game_engine_queue_multiplayer_sound(arm_sound[adjusted_type - 0x0e], 0xffffffff, 0);
            }
            break;
        }
        case 0x13: {
            // T[19]: case U -- two datum_get guards, then get_score, format, success directly
            // (no announcer sound on this one, unlike the P..T group).
            void *a = datum_get(subject, player_data);
            void *b = datum_get(subject, player_data); // UNSURE: second handle unknown
            if (a == 0 || b == 0) {
                ok = 0;
                break;
            }
            if (current_game_engine != 0 && current_game_engine->get_score != 0) {
                ((int32_t (*)(datum_index))current_game_engine->get_score)(subject);
            }
            {
                datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                string_format_wide_va_bounded(out, fmt, subject, subject); // UNSURE args
            }
            break;
        }
        case 0x17: case 0x18: case 0x1a: case 0x1b: {
            // T[23,24,26,27]: cases V/W/Y/Z -- plain lookup-and-copy, empty string on failure.
            datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
            if (tag_id != k_datum_index_none) {
                wchar_t *text = text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                wcsncpy(out, text, buffer_size);
            } else {
                wcsncpy(out, L"", buffer_size);
            }
            break;
        }
        case 0x19: {
            // T[25]: case X -- format with or without a looked-up prefix, no wcsncpy fallback.
            datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
            if (tag_id == k_datum_index_none) {
                string_format_wide_va_bounded(out, L"%d", subject); // UNSURE fmt/args
            } else {
                wchar_t *fmt = text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                string_format_wide_va_bounded(out, fmt, subject); // UNSURE args
            }
            break;
        }
        case 0x1d: {
            // T[29]: case AA -- a 140-byte helper record built by input_get_last_used_binding; empty string if
            // it reports failure, otherwise a second helper call then the usual lookup+format.
            uint8_t scratch[140];
            if (!input_get_last_used_binding(scratch)) {
                out[0] = 0;
            } else {
                input_get_binding_display_name();
                {
                    datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                    wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                        : text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                    string_format_wide_va_bounded(out, fmt, subject); // UNSURE args
                }
            }
            break;
        }
        case 0x1e: {
            // T[30]: case AB -- a "time remaining" style message: looked-up prefix concatenated
            // with a formatted tick count.
            datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
            wchar_t *prefix = (tag_id == k_datum_index_none) ? (wchar_t *)L""
                : text_string_list_get_string(tag_id, (int16_t)adjusted_type);
            int32_t formatted_len;
            // objdump 0x45f0fe..0x45f10d: ECX = the function's own `subject` (the tick count
            // this message carries), and the two pushed arguments are `buffer_size` and `out`.
            game_time_format_minutes_seconds((uint32_t)subject, buffer_size, out);
            formatted_len = (int32_t)wcslen(out);
            wcsncat(out, prefix, buffer_size - formatted_len);
            break;
        }
        case 0x1f: {
            // T[31]: case AC -- plain lookup-and-copy, empty string on failure (same shape as
            // the V/W/Y/Z group but its own fallback label in the original).
            datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
            if (tag_id != k_datum_index_none) {
                wchar_t *text = text_string_list_get_string(tag_id, (int16_t)adjusted_type);
                wcsncpy(out, text, buffer_size);
            } else {
                wcsncpy(out, L"", buffer_size);
            }
            break;
        }
        default:
            // T[20,21,22]: unreachable/failure arm.
            ok = 0;
            break;
        }
    } else {
        ok = 0;
    }

    out[buffer_size - 1] = 0;
    return ok;
}

#if 0
Original Ghidra decompilation (0x45e680), from tools/pack.py 0x45e680:

undefined1 game_engine_build_kill_feed_message_text(uint param_1,undefined4 param_2,size_t param_3)

{
  char cVar1;
  int iVar2;
  int iVar3;
  wchar_t *pwVar4;
  wchar_t *unaff_EBX;
  undefined1 local_8d;
  undefined1 auStack_8c [140];

  local_8d = 1;
  if (((DAT_006f1d20 == 0) || (*(code **)(DAT_006f1d20 + 0x84) == (code *)0x0)) ||
     (cVar1 = (**(code **)(DAT_006f1d20 + 0x84))(1), cVar1 == '\0')) {
switchD_0045e6cb_default:
    if (param_1 < 0x20) goto LAB_0045e72e;
    goto switchD_0045e735_caseD_45f15e;
  }
  switch(param_1) {
  case 7:
    param_1 = 0x10;
    break;
  case 8:
    param_1 = 0x13;
    break;
  case 9:
    param_1 = 0xf;
    break;
  case 10:
    param_1 = 0xe;
    break;
  case 0xb:
    param_1 = 0x12;
    break;
  case 0xc:
    param_1 = 0x11;
    break;
  default:
    goto switchD_0045e6cb_default;
  }
LAB_0045e72e:
  switch((&switchD_0045e735::switchdataD_0045f198)[param_1]) {
  // The jump table this switch reads (0x45f198..0x45f214, 32 dwords) was extracted directly from
  // the binary; see the file header note. Ghidra's own rendering of each arm (as a "case
  // (undefined *)0x45eXXX:" label) is preserved in the full pack (out/phase2/game/*.md,
  // tools/pack.py 0x45e680) rather than reproduced a second time here for space.
  }
switchD_0045e735_caseD_45f15e:
  local_8d = 0;
LAB_0045f163:
  unaff_EBX[param_3 - 1] = L'\0';
  return local_8d;
}
#endif
