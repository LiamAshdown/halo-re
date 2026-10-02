// input_menu_generate_events  (Ghidra: FUN_0048ec50, renamed)
// address 0x48ec50, size 2938 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/input_types_notes.md corrects the earlier "movement double-tap detector"
// summary: "the menu navigation event generator (arrow keys / numpad, enter / escape, accept /
// back buttons, sticks past 0x7ff, mouse click and double click into the UI event queue)."
// Clears local_player_input_state[0] (states[0]), then for the keyboard (all
// k_control_keyboard_key_count keys), then each of the 4 gamepad slots (accept/back button
// shortcut, every button, every axis, every POV), builds a "menu direction" edge/repeat signal
// for up/down/left/right (driven by whichever action -- movement or the paired look action -- is
// bound: forward/look_up -> up, backward/look_down -> down, left/look_left -> left,
// right/look_right -> right) using the four menu_repeat_states entries and
// k_input_menu_repeat_ms, and single-press accept/back signals. A single monotonically
// increasing "virtual key id" (spanning the keyboard scan then every gamepad button/axis/POV in
// turn) lets a repeat slot stay claimed by whichever physical input first triggered it. Once
// every device is scanned, pushes queue-0 UI events (kind 3 for keyboard/gamepad, 4 for mouse)
// for each signal that fired, then separately handles Insert/Delete key shortcuts, a left-button
// click, a left-button double-click (GetDoubleClickTime-timed), and a right-button click.
// objdump of 0x48f4e0..0x48f7d8 was used to pin every one of these push_event calls (queue index
// 0, hidden in EAX) and their exact kind/code/pressed bytes, since Ghidra's pseudo-C shows none
// of push_event's arguments.
// UNSURE: the Insert/Delete event codes (3 and 2) and the accept/back codes (0 and 0xd) are
// reproduced literally; their meaning at the UI widget layer is outside this module.
// register convention: no parameters, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern input_abstraction_globals input_globals;    // 0x00710328
extern input_event_queue input_event_queue_active;              // 0x00712cc0
extern int16_t keyboard_bindings[k_control_keyboard_key_count]; // 0x00710330
extern int16_t gamepad_action_buttons[k_control_gamepad_count][2]; // 0x00710526
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];  // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736
extern int32_t joystick_slot_devices[4];           // 0x006b2ce8
extern input_device input_devices[8];              // 0x006b1868
extern uint8_t input_suppressed;                   // 0x006b15f9
extern joystick_state joystick_states[4];          // 0x006b2a68
extern joystick_state joystick_neutral_state;      // 0x006b2cf8
extern menu_repeat_state menu_repeat_states[4];    // 0x0068e4fc, up/down/left/right
extern void *mouse_device;                         // 0x006b1804
extern mouse_state live_mouse_state;               // 0x006b180c
extern uint32_t mouse_double_click_time;           // 0x00712c28
extern int64_t performance_frequency;              // 0x006ac8f8/0x006ac8fc

extern uint8_t input_get_key_state(int16_t key_index); // this module, 0x490b50, blam-cc: ECX
extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // this module, 0x492340

// blam-cc: virtual_key_id claims/releases one direction's repeat slot; see file header.
static void menu_direction_update(menu_repeat_state *state, uint8_t active, int32_t now_ms,
                                   int32_t virtual_key_id, uint8_t *fired)
{
    if (state->key != -1 && virtual_key_id != state->key) {
        return;
    }
    if (active) {
        if ((uint32_t)(now_ms - (int32_t)state->last_event_time) > k_input_menu_repeat_ms) {
            *fired = 1;
            state->last_event_time = now_ms;
            state->key = virtual_key_id;
        }
    } else {
        state->last_event_time = 0;
        state->key = -1;
    }
}

// Dispatches one action (forward/backward/left/right, or their paired look_* action) to the
// matching menu direction's repeat state, if it is one of those eight actions.
static void menu_direction_dispatch(int16_t action, uint8_t active, int32_t now_ms,
                                     int32_t virtual_key_id, uint8_t fired[4])
{
    switch (action) {
    case _input_action_forward:
    case _input_action_look_up:
        menu_direction_update(&menu_repeat_states[0], active, now_ms, virtual_key_id, &fired[0]);
        break;
    case _input_action_backward:
    case _input_action_look_down:
        menu_direction_update(&menu_repeat_states[1], active, now_ms, virtual_key_id, &fired[1]);
        break;
    case _input_action_left:
    case _input_action_look_left:
        menu_direction_update(&menu_repeat_states[2], active, now_ms, virtual_key_id, &fired[2]);
        break;
    case _input_action_right:
    case _input_action_look_right:
        menu_direction_update(&menu_repeat_states[3], active, now_ms, virtual_key_id, &fired[3]);
        break;
    default:
        break;
    }
}

static void push_menu_event(int16_t kind, uint8_t code, uint8_t pressed)
{
    ui_input_event event;

    // the binary builds the record in a reused stack slot ([esp+0x28]); controller_index is
    // written by the push and axis_y (+0x06) keeps stale bytes there, zeroed here
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.code = code;
    event.pressed = pressed;
    input_queue_push_event(0, &event);
}

void input_menu_generate_events(void)
{
    large_integer counter;
    int32_t now_ms;
    uint8_t fired[4]; // up, down, left, right
    uint8_t accept_fired;
    uint8_t back_fired;
    int32_t virtual_id;
    int32_t key_index;
    int32_t slot;
    int32_t dev;
    int32_t i;
    joystick_state *source;
    int16_t axis_value;
    int16_t axis_action;
    uint8_t axis_active;
    int32_t octant;
    uint8_t held;

    fired[0] = 0;
    fired[1] = 0;
    fired[2] = 0;
    fired[3] = 0;
    accept_fired = 0;
    back_fired = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));

    virtual_id = 0;
    for (key_index = 0; key_index < k_control_keyboard_key_count; key_index++) {
        held = input_get_key_state((int16_t)key_index);
        virtual_id++;
        switch (key_index) {
        case _input_key_escape:
            if (held == 1) {
                back_fired = 1;
            }
            break;
        case _input_key_enter:
        case _input_key_numpad_enter:
            if (held == 1) {
                accept_fired = 1;
            }
            break;
        case _input_key_up:
        case _input_key_numpad_8:
            menu_direction_update(&menu_repeat_states[0], held != 0, now_ms, virtual_id, &fired[0]);
            break;
        case _input_key_down:
        case _input_key_numpad_2:
            menu_direction_update(&menu_repeat_states[1], held != 0, now_ms, virtual_id, &fired[1]);
            break;
        case _input_key_left:
        case _input_key_numpad_4:
            menu_direction_update(&menu_repeat_states[2], held != 0, now_ms, virtual_id, &fired[2]);
            break;
        case _input_key_right:
        case _input_key_numpad_6:
            menu_direction_update(&menu_repeat_states[3], held != 0, now_ms, virtual_id, &fired[3]);
            break;
        default:
            if (keyboard_bindings[key_index] == _input_action_accept) {
                if (held == 1) {
                    accept_fired = 1;
                }
            } else if (keyboard_bindings[key_index] == _input_action_back) {
                if (held == 1) {
                    back_fired = 1;
                }
            }
            break;
        }
    }

    for (slot = 0; slot < 4; slot++) {
        dev = joystick_slot_devices[slot];
        source = (dev == -1) ? (joystick_state *)0
                 : ((input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state);

        if (dev != -1 && gamepad_action_buttons[slot][0] != -1 &&
            source->button_frames[gamepad_action_buttons[slot][0]] == 1) {
            accept_fired = 1;
        }
        if (dev != -1 && gamepad_action_buttons[slot][1] != -1 &&
            source->button_frames[gamepad_action_buttons[slot][1]] == 1) {
            back_fired = 1;
        }

        for (i = 0; dev != -1 && i < input_devices[dev].button_count; i++) {
            held = source->button_frames[i];
            virtual_id++;
            menu_direction_dispatch(gamepad_button_bindings[slot][i], held != 0, now_ms, virtual_id, fired);
        }

        for (i = 0; dev != -1 && i < input_devices[dev].axis_count; i++) {
            axis_value = source->axes[i];
            virtual_id++;
            if (axis_value < 0) {
                axis_action = gamepad_axis_bindings[slot][i][1];
                axis_active = axis_value < -k_input_menu_axis_threshold;
            } else if (axis_value > 0) {
                axis_action = gamepad_axis_bindings[slot][i][0];
                axis_active = axis_value > k_input_menu_axis_threshold;
            } else {
                continue;
            }
            if (axis_action != k_control_binding_unbound) {
                menu_direction_dispatch(axis_action, axis_active, now_ms, virtual_id, fired);
            }
        }

        for (i = 0; dev != -1 && i < input_devices[dev].pov_count; i++) {
            virtual_id++;
            for (octant = 0; octant < 8; octant++) {
                int16_t pov_action = gamepad_pov_bindings[slot][i][octant];
                if (pov_action != k_control_binding_unbound) {
                    menu_direction_dispatch(pov_action, octant == source->povs[i], now_ms, virtual_id, fired);
                }
            }
        }
    }

    if (fired[0] && input_event_queue_active.enabled) {
        push_menu_event(3, 8, 1);
    }
    if (fired[1] && input_event_queue_active.enabled) {
        push_menu_event(3, 9, 1);
    }
    if (fired[2] && input_event_queue_active.enabled) {
        push_menu_event(3, 0xa, 1);
    }
    if (fired[3] && input_event_queue_active.enabled) {
        push_menu_event(3, 0xb, 1);
    }
    if (accept_fired && input_event_queue_active.enabled) {
        push_menu_event(3, 0, 1);
    }
    if (back_fired && input_event_queue_active.enabled) {
        push_menu_event(3, 0xd, 1);
    }

    held = input_get_key_state(_input_key_insert);
    if (held != 0 && input_event_queue_active.enabled) {
        held = input_get_key_state(_input_key_insert);
        push_menu_event(3, 3, held);
    }

    held = input_get_key_state(_input_key_delete);
    if (held != 0 && input_event_queue_active.enabled) {
        held = input_get_key_state(_input_key_delete);
        push_menu_event(3, 2, held);
    }

    if (mouse_device != 0 && input_suppressed == 0 && live_mouse_state.button_frames[0] != 0 &&
        input_event_queue_active.enabled) {
        push_menu_event(4, 0, live_mouse_state.button_frames[0]);
    }

    {
        uint32_t double_click_ms = GetDoubleClickTime();
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

        if (mouse_double_click_time == 0) {
            if (live_mouse_state.button_pressed[0] != 0) {
                mouse_double_click_time = now_ms;
            }
        } else if (live_mouse_state.button_pressed[0] == 0) {
            if ((uint32_t)(now_ms - mouse_double_click_time) >= double_click_ms) {
                mouse_double_click_time = 0;
            }
        } else {
            if (input_event_queue_active.enabled) {
                push_menu_event(4, 3, 1);
            }
            mouse_double_click_time = 0;
        }
    }

    // 0x48f7a5: the right button is mouse_state slot 2 (mouse_button_map puts the physical
    // right button there), read as its hold count at 0x006b181a, not a went-down flag
    if (mouse_device != 0 && input_suppressed == 0 && live_mouse_state.button_frames[2] != 0 &&
        input_event_queue_active.enabled) {
        push_menu_event(4, 2, live_mouse_state.button_frames[2]);
    }
}

#if 0
Original Ghidra decompilation (0x48ec50):

void FUN_0048ec50(void)

{
  short sVar1;
  short sVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;
  char cVar10;
  undefined1 uVar11;
  int iVar12;
  undefined4 *puVar13;
  short *psVar14;
  UINT UVar15;
  undefined2 *puVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  undefined8 uVar20;
  int local_18;
  undefined4 *local_14;
  short *local_10;
  int *local_c;
  LARGE_INTEGER local_8;

  bVar4 = false;
  bVar5 = false;
  bVar6 = false;
  bVar7 = false;
  bVar8 = false;
  bVar9 = false;
  QueryPerformanceCounter(&local_8);
  uVar20 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar12 = __alldiv(uVar20,DAT_006ac8f8,DAT_006ac8fc);
  DAT_00712498 = 0;
  DAT_0071249c = 0;
  _DAT_007124a0 = 0;
  _DAT_007124a4 = 0;
  _DAT_007124a8 = 0;
  _DAT_007124ac = 0;
  _DAT_007124b0 = 0;
  _DAT_007124b4 = 0;
  _DAT_007124b8 = 0;
  iVar19 = 0;
  _DAT_007124bc = 0;
  iVar18 = 0;
  iVar17 = DAT_0068e518;
  do {
    cVar10 = FUN_00490b50();
    iVar19 = iVar19 + 1;
    switch(iVar18) {
    case 0:
switchD_0048ed15_caseD_0:
      if (cVar10 == '\x01') {
        bVar9 = true;
      }
      break;
    default:
      if ((&DAT_00710330)[iVar18] == 8) goto switchD_0048ed15_caseD_38;
      if ((&DAT_00710330)[iVar18] == 9) goto switchD_0048ed15_caseD_0;
      break;
    case 0x38:
    case 0x66:
switchD_0048ed15_caseD_38:
      if (cVar10 == '\x01') {
        bVar8 = true;
      }
      break;
    case 0x4d:
    case 0x62:
      if ((DAT_0068e500 == -1) || (iVar19 == DAT_0068e500)) {
        if (cVar10 == '\0') {
          DAT_0068e4fc = 0;
          DAT_0068e500 = -1;
        }
        else if (0x15d < (uint)(iVar12 - DAT_0068e4fc)) {
          bVar4 = true;
          DAT_0068e4fc = iVar12;
          DAT_0068e500 = iVar19;
        }
      }
      break;
    /* ... down/left/right cases identical in shape, DAT_0068e504/8, DAT_0068e50c/10,
       DAT_0068e514 + local iVar17 for DAT_0068e518 ... */
    }
    iVar18 = iVar18 + 1;
  } while (iVar18 < 0x6d);
  local_18 = 0;
  DAT_0068e518 = iVar17;
  do {
    /* per-slot: accept/back button shortcut, then button/axis/POV scans, each running the same
       repeat-state switch on cases 0x13/0x17 (up), 0x14/0x18 (down), 0x15/0x19 (left),
       0x16/0x1a (right) -- see out/phase4/input_batch/48ec50.md for the full decompile. */
    local_18 = local_18 + 1;
  } while (local_18 < 4);
  /* event firing: see objdump 0x48f4e0..0x48f7d8, quoted in the file header, for the exact
     kind/code/pressed bytes push_event receives (queue index 0 in every call, hidden in EAX). */
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
