// saved_item_select  (Ghidra: FUN_00495be0, unnamed)
// address 0x495be0, size 169 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/interface_functions.md "Selects a saved map or game-variant entry (by
// combined type/id) into the profile's current working-copy."; types/interface.h's globals list
// ("selected_saved_item, low nibble 0 map, 1 variant", "saved_item_working_copy",
// "saved_item_disk_copy"); src/game/game_engine_get_variant_by_name.c's precedent signature for
// saved_game_get_variant(slot, game_variant *out); player_profile_subsystem_initialize.c's
// default_profile_data (0x0071d280, the same 0x1ffc byte template copied here for the "no
// selection" map case).
// register convention: combined type/id in EBX (unaff_EBX). // blam-cc: EBX -> item
// Type 0 is a player profile, not a map: player_profile_save (0x495d40) reloads a type-0
// selection through player_profile_load, and the -1 case copies the default profile template. Review pass (phase 4): the disassembly confirms both loaders write into
// saved_item_disk_copy: player_profile_get gets it in ECX (0x495c5e) and saved_game_get_variant as its
// second pushed argument (0x495c09).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item;          // 0x00714e7c
extern uint8_t saved_item_disk_copy[0x1ffc]; // 0x00716e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern uint32_t unknown_00714eb8;            // 0x00714eb8, UNSURE: flags, bits 0x180 cleared here
extern uint8_t default_profile_data[0x1ffc]; // 0x0071d280 (per player_profile_subsystem_initialize.c)

extern uint8_t player_profile_get(int32_t slot, void *out_record); // 0x53a770; blam-cc: ECX -> out_record
extern void game_engine_apply_current_custom_variant(void); // 0x463b90
extern uint8_t saved_game_get_variant(int32_t slot, game_variant *out); // 0x53bee0, UNSURE

// blam-cc: EBX -> item
// Selects saved item `item` (low nibble 0 = map, 1 = variant; -1 selects "nothing"/the default)
// into the working copy: for a map, either resets to the compiled-in default template or, if
// `item` validates, copies the whole 0x1ffc byte disk record over; for a variant, either
// re-applies the current custom variant or, if `item` validates, loads it into the disk copy and
// copies just the 0x98 byte game_variant portion over, clearing two flag bits in the process.
// Does nothing (selected_saved_item keeps its reset value of -1) if validation fails.
void saved_item_select(int32_t item)
{
    selected_saved_item = -1;

    if ((item & 0xf) == 0) {
        if (item == -1) {
            memcpy(saved_item_disk_copy, default_profile_data, sizeof(default_profile_data));
            return;
        }
        if (player_profile_get(item, saved_item_disk_copy) != 0) {
            memcpy(saved_item_working_copy, saved_item_disk_copy, sizeof(saved_item_working_copy));
            selected_saved_item = item;
        }
    } else if ((item & 0xf) == 1) {
        if (item == -1) {
            game_engine_apply_current_custom_variant();
            return;
        }
        if (saved_game_get_variant(item, (game_variant *)saved_item_disk_copy) != 0) {
            memcpy(saved_item_working_copy, saved_item_disk_copy, sizeof(game_variant));
            unknown_00714eb8 = unknown_00714eb8 & 0xfffffe7f;
            selected_saved_item = item;
        }
    }
}

#if 0
Original Ghidra decompilation (0x495be0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00495be0(void)

{
  char cVar1;
  int iVar2;
  uint unaff_EBX;
  undefined4 *puVar3;
  undefined4 *puVar4;

  _DAT_00714e7c = 0xffffffff;
  if ((unaff_EBX & 0xf) == 0) {
    if (unaff_EBX == 0xffffffff) {
      puVar3 = &DAT_0071d280;
      puVar4 = &DAT_00716e7c;
      for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
      return;
    }
    cVar1 = player_profile_get();
    if (cVar1 != '\0') {
      puVar3 = &DAT_00716e7c;
      puVar4 = &DAT_00714e80;
      for (iVar2 = 0x7ff; _DAT_00714e7c = unaff_EBX, iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
    }
  }
  else if ((unaff_EBX & 0xf) == 1) {
    if (unaff_EBX == 0xffffffff) {
      game_engine_apply_current_custom_variant();
      return;
    }
    cVar1 = FUN_0053bee0();
    if (cVar1 != '\0') {
      puVar3 = &DAT_00716e7c;
      puVar4 = &DAT_00714e80;
      for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
      DAT_00714eb8 = DAT_00714eb8 & 0xfffffe7f;
      _DAT_00714e7c = unaff_EBX;
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
