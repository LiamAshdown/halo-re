// weapon_hud_ammo_state_is_empty  (Ghidra: FUN_004a9750, renamed; earlier
// ui_zoom_meter_state_is_default)
// address 0x4a9750, size 39 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: the record is the weapon_hud_ammo_state of types/items.h, built by
// weapon_build_hud_ammo_state @0x4c29d0 into the same stack slot right before every call in
// hud_update_interaction_prompt @0x4a9b80. Offsets 0x0e, 0x10 and 0x12 are
// magazines[0].rounds_loaded, rounds_loaded_maximum and rounds_unloaded, 0x04 is age. It is true
// for a magazine weapon with nothing loaded or in reserve, or for a battery weapon at age 1.0,
// and the caller then looks for another weapon to switch to. Checked against objdump.
// register convention: state pointer in EAX.
//   // blam-cc: state -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: state -> EAX
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t weapon_hud_ammo_state_is_empty(const weapon_hud_ammo_state *state)
{
    const weapon_hud_magazine_state *magazine = &state->magazines[0];

    if (magazine->rounds_loaded_maximum != 0 && magazine->rounds_loaded == 0 &&
        magazine->rounds_unloaded == 0) {
        return 1;
    }
    return state->age == 1.0f; // compared as the dword 0x3f800000
}

#if 0
Original Ghidra decompilation (0x4a9750):

undefined4 FUN_004a9750(void)

{
  int in_EAX;

  if ((((*(short *)(in_EAX + 0x10) == 0) || (*(short *)(in_EAX + 0xe) != 0)) ||
      (*(short *)(in_EAX + 0x12) != 0)) && (*(int *)(in_EAX + 4) != 0x3f800000)) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
