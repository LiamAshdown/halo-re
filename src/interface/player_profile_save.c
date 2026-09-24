// player_profile_save  (Ghidra: player_profile_save, already named)
// address 0x495d40, size 290 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: cea-pdb hint via the "profile not saved since it was a default profile" string;
// out/phase4/interface_functions.md "Saves the currently-selected map or game-variant profile
// entry to disk, refusing to overwrite an unm[atched/named default]."; reuses saved_item_select.c's
// selected_saved_item/saved_item_working_copy/saved_item_disk_copy.
// register convention: none; selected_saved_item is read directly.
// Review pass (phase 4): rebuilt from the disassembly. The type-0 (profile) branch reloads from
// saved_item_working_copy (EDX = 0x714e80, EAX = player 0, stack = selected_saved_item), not
// from the profile globals. The variant branches call game_variant_sanitize_options with
// ECX = 0x714e80 and then push ECX unchanged as game_variant_write_request_start's second argument, and
// saved_game_get_directory_by_handle takes the slot in EAX and a 0x100 byte local name buffer in ESI, which is then
// pushed to saved_game_last_mp_variant_clear. 0x714f14 is cleared with a byte AND.
// UNSURE: game_variant_sanitize_options, saved_game_create_custom_variant, game_variant_write_request_start, saved_game_get_directory_by_handle and
// saved_game_last_mp_variant_clear are profile/saved-game module functions not rewritten here;
// the ECX-preserved argument to game_variant_write_request_start assumes 0x466730 leaves ECX intact, as the
// compiler evidently did.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern int32_t selected_saved_item;             // 0x00714e7c
extern uint8_t saved_item_disk_copy[0x1ffc];     // 0x00716e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern uint8_t unknown_00714f14;                 // 0x00714f14, UNSURE: working copy +0x94 flags byte

extern void console_out_printf(uint8_t unknown, const char *format, ...); // 0x4c6860
extern void player_profile_write_data(int32_t slot, void *profile_data);  // 0x53a950
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970
extern void game_variant_sanitize_options(game_variant *variant); // 0x466730; blam-cc: ECX -> variant
extern void game_variant_write_request_start(int32_t slot, void *variant);    // 0x53c0b0, UNSURE
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name); // 0x53d080; blam-cc: EAX -> slot, ESI -> out_name
extern void saved_game_last_mp_variant_clear(char *name);  // 0x53d360
extern int32_t saved_game_create_custom_variant(int32_t unknown, void *variant_data); // 0x53bb50, UNSURE

// Saves the currently-selected saved profile (type 0) or game variant (type 1): a profile is
// written back to its slot (or a "not saved" message for the default -1 profile) and reloaded as
// player 0's profile; a variant is sanitized and persisted, first allocating a new slot if it
// was an in-memory-only (+0x40000000) variant whose name differs from its disk copy.
// selected_saved_item is reset to -1 on every path. Returns 1 on a save, 0 when nothing was done.
uint8_t player_profile_save(void)
{
    char name[0x100];
    int32_t item;
    int32_t new_slot;
    uint8_t result;

    item = selected_saved_item;
    result = 0;

    if ((item & 0xf) == 0) {
        if (item == -1) {
            console_out_printf(0, "profile not saved since it was a default profile");
        } else {
            player_profile_write_data(item, saved_item_working_copy);
        }
        player_profile_load(0, saved_item_working_copy, selected_saved_item);
        result = 1;
    } else if ((item & 0xf) == 1) {
        if ((item & 0x40000000) == 0) {
            if (item != -1) {
                game_variant_sanitize_options((game_variant *)saved_item_working_copy);
                game_variant_write_request_start(item, saved_item_working_copy);
            }
            if (saved_game_get_directory_by_handle(selected_saved_item, name) != 0) {
                saved_game_last_mp_variant_clear(name);
            }
            result = 1;
        } else if (wcsncmp((wchar_t *)saved_item_working_copy, (wchar_t *)saved_item_disk_copy,
                           0x18) != 0) {
            unknown_00714f14 &= 0xfe;
            new_slot = saved_game_create_custom_variant(0, saved_item_working_copy);
            if (new_slot != -1) {
                game_variant_sanitize_options((game_variant *)saved_item_working_copy);
                game_variant_write_request_start(new_slot, saved_item_working_copy);
                selected_saved_item = new_slot;
                if (saved_game_get_directory_by_handle(new_slot, name) != 0) {
                    saved_game_last_mp_variant_clear(name);
                }
                result = 1;
            }
        }
    }

    selected_saved_item = -1;
    return result;
}

#if 0
Original Ghidra decompilation (0x495d40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 player_profile_save(void)

{
  uint uVar1;
  char cVar2;
  int iVar3;
  undefined4 extraout_ECX;
  undefined4 extraout_ECX_00;

  uVar1 = _DAT_00714e7c;
  if ((_DAT_00714e7c & 0xf) == 0) {
    if (_DAT_00714e7c == 0xffffffff) {
      console_out_printf('\0',"profile not saved since it was a default profile");
    }
    else {
      player_profile_write_data(_DAT_00714e7c,&DAT_00714e80);
    }
    player_profile_load(_DAT_00714e7c);
  }
  else {
    if ((_DAT_00714e7c & 0xf) != 1) {
      _DAT_00714e7c = 0xffffffff;
      return 0;
    }
    if ((_DAT_00714e7c & 0x40000000) == 0) {
      if (_DAT_00714e7c != 0xffffffff) {
        game_variant_sanitize_options();
        FUN_0053c0b0(uVar1,extraout_ECX_00);
      }
      cVar2 = FUN_0053d080();
      if (cVar2 != '\0') {
        saved_game_last_mp_variant_clear();
      }
    }
    else {
      iVar3 = _wcsncmp((wchar_t *)&DAT_00714e80,(wchar_t *)&DAT_00716e7c,0x18);
      if (iVar3 == 0) {
        _DAT_00714e7c = 0xffffffff;
        return 0;
      }
      DAT_00714f14._0_1_ = (byte)DAT_00714f14 & 0xfe;
      iVar3 = FUN_0053bb50(0,&DAT_00714e80);
      if (iVar3 == -1) {
        _DAT_00714e7c = 0xffffffff;
        return 0;
      }
      game_variant_sanitize_options();
      FUN_0053c0b0(iVar3,extraout_ECX);
      _DAT_00714e7c = iVar3;
      cVar2 = FUN_0053d080();
      if (cVar2 != '\0') {
        saved_game_last_mp_variant_clear();
      }
    }
  }
  _DAT_00714e7c = 0xffffffff;
  return 1;
}
#endif
