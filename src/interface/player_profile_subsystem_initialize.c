// player_profile_subsystem_initialize  (Ghidra: FUN_00495370, unnamed)
// address 0x495370, size 372 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Initializes the player-profile subsystem: clears
// profile globals and loads (or creates/saves) the appropriate profile at startup."; cea-pdb hint
// "player_profile_save" via the "profile not saved since it was a default profile" string (that
// hint names the whole save/init family, not this specific function -- kept as evidence only).
// register convention: none (void).
// Review pass (phase 4): rebuilt from the disassembly (0x495370..0x4954e3). Ghidra lost the
// 0x2008 byte frame: L+0 is the capacity-then-count int32 saved_game_enumerate_by_type takes in
// EBX (1 in), L+4 the one enumerated slot, and L+8 the 0x1ffc byte profile record that
// player_profile_get (ECX), 0x53b000, 0x53b240 (EAX), player_profile_load (EDX) and
// player_profile_write_data all share. The enumeration result is only used when the returned
// count is positive, and player_profile_load takes player index 0 in EAX and the slot on the
// stack. player_profile_refresh_settings_cache is called with BX = 0.
// UNSURE: the profile-module callees (player_profile_initialize, player_profile_get,
// player_profile_write_data, player_profile_set_default_video_options, player_profile_set_default_audio_options,
// saved_game_last_profile_read, saved_game_find_by_name) are foreign; argument order is from the
// push sequence here, their meaning from the names already in symbols/functions.txt.
// UNSURE: DAT_0071d280 is a 0x1ffc byte record copied into the local buffer when no cached slot
// exists (a compiled-in default profile), which the enumerated-slot path then overwrites.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8, 3 x 0x2004 byte profile records + tail
extern int32_t current_profile_index;         // 0x00714dd4
extern int16_t profile_slot_id[];             // 0x00714dde
extern int32_t selected_saved_item;           // 0x00714e7c
extern uint8_t default_profile_data[0x1ffc]; // 0x0071d280, UNSURE: compiled-in default profile
extern uint8_t saved_game_index_dirty;        // 0x00721447, UNSURE name: see symbols 0x53c4e0
extern char last_profile_name[];              // 0x00718e80, name buffer, [0] != 0 when valid
extern int32_t cached_profile_slot;           // 0x0068e66c, UNSURE
extern int32_t profile_write_back_enabled;    // 0x007196f4, UNSURE
extern uint8_t profile_load_complete;         // 0x00718e78, UNSURE

extern void player_profile_refresh_settings_cache(int16_t player_index); // 0x496060, BX
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970

extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, stack (type, out, builtin_only), EBX &count
    // 0x53c4e0; blam-cc: EBX -> capacity-then-count int32 (see game_engine_get_variant_by_name.c)
extern void player_profile_initialize(void *profile_globals, int32_t unknown_0, int32_t unknown_1); // 0x53a1c0
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile
extern uint8_t saved_game_last_profile_read(char *name_buffer);      // 0x53d2b0
extern int32_t saved_game_find_by_name(char *name, int32_t unknown); // 0x53d4a0
extern void player_profile_set_default_video_options(void *profile_data, int32_t unknown); // 0x53b000
extern void player_profile_set_default_audio_options(void *profile_data);                        // 0x53b240; blam-cc: EAX -> profile_data
extern void player_profile_write_data(int32_t slot, void *profile_data); // 0x53a950
extern void console_out_printf(uint8_t unknown, const char *format, ...); // 0x4c6860

// Initializes the player-profile subsystem at startup: clears the profile module's scratch
// block, resets the current-profile globals, then picks a profile to load: the cached slot if
// player_profile_get accepts it, else the first type-0 enumerated slot if the enumeration found
// one and it validates. When write-back is enabled, seeds default video options before the load
// and writes the record back to the enumerated slot afterwards (with a console message instead
// when there is no enumerated slot).
void player_profile_subsystem_initialize(void)
{
    int32_t enumerated_count;          // L+0
    int32_t enumerated_slot;           // L+4
    uint8_t profile_data[0x1ffc];      // L+8
    int32_t slot_to_load;

    memset(profile_globals_block, 0, sizeof(profile_globals_block));
    player_profile_initialize(profile_globals_block, 0, 0);
    current_profile_index = -1;
    profile_slot_id[0] = -1;
    player_profile_refresh_settings_cache(0);
    selected_saved_item = -1;

    enumerated_count = 1;
    enumerated_slot = -1;
    saved_game_enumerate_by_type(0, &enumerated_slot, 0, (uint16_t *)&enumerated_count); // EBX = &enumerated_count
    saved_game_index_dirty = 1;

    if (last_profile_name[0] == '\0' && saved_game_last_profile_read(last_profile_name) != 0) {
        cached_profile_slot = saved_game_find_by_name(last_profile_name, 0);
    }

    slot_to_load = cached_profile_slot;
    if (slot_to_load == -1) {
        memcpy(profile_data, default_profile_data, sizeof(profile_data));
    } else if (player_profile_get(slot_to_load, profile_data) != 0) {
        goto have_slot;
    }

    if ((int16_t)enumerated_count <= 0 || enumerated_slot == -1 ||
        player_profile_get(enumerated_slot, profile_data) == 0) {
        profile_load_complete = 1;
        return;
    }
    slot_to_load = enumerated_slot;

have_slot:
    if (slot_to_load != -1) {
        if (profile_write_back_enabled != 0) {
            player_profile_set_default_video_options(profile_data, 0);
            player_profile_set_default_audio_options(profile_data);
        }
        player_profile_load(0, profile_data, slot_to_load);
        if (profile_write_back_enabled != 0) {
            if (enumerated_slot == -1) {
                console_out_printf(0, "profile not saved since it was a default profile");
                profile_load_complete = 1;
                return;
            }
            player_profile_write_data(enumerated_slot, profile_data);
        }
    }
    profile_load_complete = 1;
}

#if 0
Original Ghidra decompilation (0x495370):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00495370(void)

{
  int iVar1;
  char cVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int local_200c;
  undefined4 local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x495380;
  puVar4 = &DAT_00712dd8;
  for (iVar3 = 0x1829; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  player_profile_initialize(&DAT_00712dd8,0,0);
  DAT_00714dd4 = 0xffffffff;
  _DAT_00714dde = 0xffff;
  FUN_00496060();
  _DAT_00714e7c = 0xffffffff;
  local_200c = -1;
  saved_game_enumerate_by_type(0,&local_200c,0);
  DAT_00721447 = 1;
  if ((DAT_00718e80 == '\0') && (cVar2 = saved_game_last_profile_read(0x718e80), cVar2 != '\0')) {
    DAT_0068e66c = saved_game_find_by_name(&DAT_00718e80,0);
  }
  iVar1 = local_200c;
  iVar3 = DAT_0068e66c;
  if (DAT_0068e66c == -1) {
    puVar4 = &DAT_0071d280;
    puVar5 = local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
  }
  else {
    cVar2 = player_profile_get(DAT_0068e66c);
    if (cVar2 != '\0') goto LAB_0049546a;
  }
  if (iVar1 == -1) {
    DAT_00718e78 = 1;
    return;
  }
  cVar2 = player_profile_get(iVar1);
  iVar3 = iVar1;
  if (cVar2 == '\0') {
    DAT_00718e78 = 1;
    return;
  }
LAB_0049546a:
  if (iVar3 != -1) {
    if (DAT_007196f4 != 0) {
      player_profile_set_default_video_options(local_2008,0);
      FUN_0053b240();
    }
    player_profile_load(iVar3);
    if (DAT_007196f4 != 0) {
      if (iVar1 == -1) {
        console_out_printf('\0',"profile not saved since it was a default profile");
        DAT_00718e78 = 1;
        return;
      }
      player_profile_write_data(iVar1,local_2008);
    }
  }
  DAT_00718e78 = 1;
  return;
}
#endif
