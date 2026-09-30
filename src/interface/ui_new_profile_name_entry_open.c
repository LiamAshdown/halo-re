// ui_new_profile_name_entry_open  (Ghidra: FUN_004a1940, renamed)
// renamed from FUN_004a1940 in the naming pass
// address 0x4a1940, size 91 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: functions.md: "Prepares a default player name and opens the name-entry text input for
// a specific player/controller slot when creating a new profile."
// register convention: cdecl, param_1 unused, param_2 a per-player context (offset +2 holds the
// player/controller index, same shape as ui_input_event used throughout this module).
// UNSURE: saved_game_allocate_new_slot's output buffer argument is inferred (Ghidra shows no
// visible arguments at all for that call, but local_100 is otherwise never written before its use
// as the wcsncpy source).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint16_t new_profile_name_buffer_006b37f4[0xc]; // 0x006b37f4, 11 characters plus the terminator at 0x006b380a
extern int16_t new_profile_name_entry_player_00692b00;  // 0x00692b00, TYPES-GAP
extern uint8_t new_profile_name_flag_0071916e;           // 0x0071916e, TYPES-GAP

extern void saved_game_allocate_new_slot(uint16_t *out_default_name); // 0x53ca80, blam-cc: EBX out_default_name
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination

uint8_t ui_new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled)
{
    uint16_t default_name[128];

    (void)widget;
    (void)out_handled;
    saved_game_allocate_new_slot(default_name);
    wcsncpy(new_profile_name_buffer_006b37f4, default_name, 0xb);
    new_profile_name_buffer_006b37f4[0xb] = 0; // word store at 0x006b380a
    new_profile_name_entry_player_00692b00 = event[1]; // offset +2
    new_profile_name_flag_0071916e = 0;
    virtual_keyboard_open(new_profile_name_buffer_006b37f4, 0x18, 8);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a1940):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004a1940(undefined4 param_1,int param_2)

{
  wchar_t local_100 [128];

  saved_game_allocate_new_slot();
  _wcsncpy(&DAT_006b37f4,local_100,0xb);
  _DAT_006b380a = 0;
  DAT_00692b00 = *(undefined2 *)(param_2 + 2);
  DAT_0071916e = 0;
  virtual_keyboard_open(0x18,8);
  return 1;
}
#endif
