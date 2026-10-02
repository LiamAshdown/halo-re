// ui_game_data_input_4a5740  (not a Ghidra function; game_data_input_function_table[11])
// address 0x4a5740, size 906 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b44 (index 11); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a5740.
// WRITTEN 2026-09-28 from objdump 0x4a5740..0x4a5ac9: multiplayer lobby screen, per frame, with a server (+8) or
//   client (+0xb14) game (the client machine word is read even without a client). Countdown label (third child, 0x20
//   byte text): client word +0xed8 seconds as L"-:--" / L"0:%02d" / L"%02d:%02d" / L"%d:%02d:%02d" (15 characters); 0
//   selects the second child's entry 1 and hides the countdown; a negative value hides both unless the game's player
//   count (+0x1a0) is at least 2 and its byte +0x138 is not 1. The first child's name label gets L"": the binary
//   wcscpy's L"" over the read-only literal L"?" at 0x0066a80c (which would fault in retail; the standalone image is
//   writable) and then copies that. Then finds the first valid player entry (16 of 0x20 bytes from +0x1a2) of this
//   machine; only a local player 0 entry is kept (the binary indexes a stack array by the player byte, overwriting
//   other locals for players 1..3). Without one: blank player name, frame 0, team list selection 0. With one: its
//   wide name (entry +0) as the player label; without teams (+0x138 == 0) frame 1, else team byte +0x1e 0 / 1 / other
//   gives frame 5 / 4 / 3 and team selection 0 / 1 / 0. Without teams the team list is disabled (state 0, every child
//   state 0).
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count
extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry
extern void widget_instance_set_state_recursive(widget_instance *widget, uint8_t state); // 0x498e60

static void set_text(widget_instance *label, const uint16_t *source)
{
    int32_t length = (int32_t)wcslen((const wchar_t *)source);
    uint16_t *text;

    text = (uint16_t *)heap_reallocate(label->text, (uint16_t)(length * 2 + 2), widget_memory_pool);
    label->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)source, length);
        text[length] = 0;
    }
}

void ui_game_data_input_4a5740(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    widget_instance *first;
    widget_instance *second;
    widget_instance *countdown;
    widget_instance *status;
    widget_instance *frame;
    widget_instance *name;
    widget_instance *team;
    int16_t key;
    int32_t found = -1;
    int32_t i;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    key = *(int16_t *)network_client;
    first = widget->first_child;
    second = first->next_sibling;
    countdown = second->next_sibling;
    text = (uint16_t *)heap_reallocate(countdown->text, 0x20, widget_memory_pool);
    countdown->text = text;
    if (text != 0) {
        int16_t seconds = *(int16_t *)(network_client + 0xed8);

        wcsncpy((wchar_t *)text, L"-:--", 0xf); // 0x0066a850
        second->state = 1;
        second->selection_index = 0;
        countdown->state = 1;
        if (seconds == 0) {
            second->selection_index = 1;
            countdown->state = 0;
        } else if (seconds > 0) {
            if (seconds < 60) {
                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"0:%02d", (int32_t)seconds);
            } else if (seconds < 3600) {
                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%02d:%02d", seconds / 60, seconds % 60);
            } else {
                int32_t hours = seconds / 3600;
                int32_t minutes = (seconds - hours * 3600) / 60;

                string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%d:%02d:%02d", hours, minutes,
                    seconds - (hours * 60 + minutes) * 60);
            }
        } else if (*(int16_t *)(game + 0x1a0) < 2 || game[0x138] == 1) {
            second->state = 0;
            countdown->state = 0;
        }
        text[0xf] = 0;
    }
    status = first->first_child;
    frame = status->next_sibling;
    name = frame->next_sibling;
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = game + 0x1a2 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key) {
            if ((int8_t)entry[0x1d] == 0) {
                found = i;
            }
            break;
        }
    }
    set_text(status, (const uint16_t *)L"");
    frame->background_bitmap_frame = 0;
    frame = name->first_child;
    team = frame->next_sibling->next_sibling;
    if (game[0x138] == 0) {
        widget_instance *child;

        team->state = 0;
        for (child = team->first_child; child != 0; child = child->next_sibling) {
            widget_instance_set_state_recursive(child, 0);
        }
    }
    if (found == -1) {
        frame->background_bitmap_frame = 0;
        text = (uint16_t *)heap_reallocate(frame->next_sibling->text, 2, widget_memory_pool);
        frame->next_sibling->text = text;
        if (text != 0) {
            text[0] = 0;
        }
        team->selection_index = 0;
        return;
    }
    set_text(frame->next_sibling, (const uint16_t *)(game + 0x1a2 + found * 0x20));
    if (game[0x138] == 0) {
        frame->background_bitmap_frame = 1;
        return;
    }
    switch ((int8_t)game[0x1a2 + found * 0x20 + 0x1e]) {
    case 0:
        frame->background_bitmap_frame = 5;
        team->selection_index = 0;
        break;
    case 1:
        frame->background_bitmap_frame = 4;
        team->selection_index = 1;
        break;
    default:
        frame->background_bitmap_frame = 3;
        team->selection_index = 0;
        break;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
