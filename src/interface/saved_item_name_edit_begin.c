// saved_item_name_edit_begin  (Ghidra: FUN_00495cf0, unnamed)
// address 0x495cf0, size 77 bytes
// name confidence: 0.35   rewrite confidence: 0.75
// evidence: out/phase4/interface_functions.md "Validates the currently-selected saved map or
// variant's name (e.g. for uniqueness/legality) before i[t can be saved]" -- actually opens the
// virtual keyboard on the name field, one screen/field id pair for a map name and another for a
// variant name; reuses saved_item_select.c's selected_saved_item. src/networking/
// server_browser_latch_join_target.c's precedent (screen_id, field_id) signature for
// virtual_keyboard_open, though that file declares it void -- this call site's return value is
// used, so it is declared uint8_t here instead.
// Phase-4 s2 review (objdump 0x495cf0..0x495d3b): the keyboard takes the destination buffer in
// ESI (0x00714e82 for a profile, 0x00714e80 for a variant) and two stack words, maximum length
// in bytes and field kind; 0x00719410 is the dword virtual_keyboard.validation_mode.
// register convention: none (void).
// UNSURE: virtual_keyboard_open's real return type/meaning and DAT_00719410
// (saved_variant_name_valid per types/interface.h's globals list, set to 2 here) are inferred
// from this call site only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item;      // 0x00714e7c
extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8 (validation_mode at 0x00719410)
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the saved item being edited

extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination

// Opens the virtual keyboard on the currently-selected saved item's name field: the map-name
// screen for a map, or the variant-name screen for a variant, in which case a successful open
// also arms saved_variant_name_valid. Returns the keyboard's own open result (map selection
// returns it directly; variant selection always returns 0 unless the open succeeded, in which
// case it returns the keyboard's nonzero result).
uint8_t saved_item_name_edit_begin(void)
{
    uint8_t opened;

    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            return virtual_keyboard_open((uint16_t *)(saved_item_working_copy + 2), 0x18, 10); // profile name
        }
        if ((selected_saved_item & 0xf) == 1) {
            opened = virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 10); // variant name
            if (opened != 0) {
                virtual_keyboard.validation_mode = 2; // dword store
                return opened;
            }
            return 0;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x495cf0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char FUN_00495cf0(void)

{
  char cVar1;
  char cVar2;

  cVar1 = '\0';
  if (_DAT_00714e7c != 0xffffffff) {
    if ((_DAT_00714e7c & 0xf) == 0) {
      cVar1 = virtual_keyboard_open(0x18,10);
    }
    else if ((_DAT_00714e7c & 0xf) == 1) {
      cVar2 = virtual_keyboard_open(0x30,10);
      cVar1 = '\0';
      if (cVar2 != '\0') {
        DAT_00719410 = 2;
        return cVar2;
      }
    }
  }
  return cVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
