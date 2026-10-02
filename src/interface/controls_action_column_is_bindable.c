// controls_action_column_is_bindable  (Ghidra: FUN_004b4df0, still unnamed there; named here)
// address 0x4b4df0, size 41 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// phase-4 review: verified against objdump 0x4b4df0..0x4b4e18; renamed from
// controls_binding_is_default (0x00692ffc is +0x14 of the 0x18 byte action table at
// 0x00692fe8, the per-column unbindable bits: 1 keyboard, 2 mouse; any other column is 1).
// evidence: out/phase4/interface_functions.md "Returns whether a given input's primary or
// secondary binding is still at its default (unmodified) value."; reuses
// controls_row_device_mask_table from controls_binding_row_widget_update.c (bit 0 = primary
// default, bit 1 = secondary default).
// register convention: ECX -> slot (0 primary, 1 secondary), EDX -> action_index.
//   // blam-cc: ECX -> slot, EDX -> action_index
// FIXED (register inputs, objdump): ECX carries slot (read at 0x4b4df0, test ecx,ecx); the
// notes wrote "slot -> ECX" (name before register, with an arrow the checker only recognizes
// register-first), so it did not parse.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t controls_row_device_mask_table[]; // 0x00692ffc, stride 0x18

// blam-cc: ECX -> slot, EDX -> action_index
uint8_t controls_action_column_is_bindable(int32_t slot, int32_t action_index)
{
    if (slot == 0) {
        return (controls_row_device_mask_table[action_index * 0x18] & 1) == 0;
    }
    if (slot != 1) {
        return 1;
    }
    return (controls_row_device_mask_table[action_index * 0x18] & 2) == 0;
}

#if 0
Original Ghidra decompilation (0x4b4df0):

bool FUN_004b4df0(void)

{
  byte bVar1;
  int in_ECX;
  int in_EDX;

  if (in_ECX == 0) {
    bVar1 = (&DAT_00692ffc)[in_EDX * 0x18] & 1;
  }
  else {
    if (in_ECX != 1) {
      return true;
    }
    bVar1 = (&DAT_00692ffc)[in_EDX * 0x18] & 2;
  }
  return bVar1 == 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
