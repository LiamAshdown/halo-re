// unit_clear_weapon_switch_state  (Ghidra: unit_update, wrongly named)
// address 0x565a70, size 50 bytes
// name confidence: 0.2 (out/phase4/units_types_notes.md: "50 bytes that duplicate the tail of
// rewrite confidence: 1.0 (FRAGMENT: 0x565a70 is the tail of unit_validate_and_clear_weapon_switch 0x5659c0..0x565aa1, whose C covers it; only its jumps reach here)
//   0x5659c0; it clears the weapon-switch flags and the 0x348 timer. Nothing to do with
//   updating a unit" -- renamed away from the misleading Ghidra name "unit_update", which
//   belongs to 0x5625b0 per the object_type_definition vtable correction)   rewrite
//   confidence: 0.2
// evidence: types/units.h unit_data.zoom_level/.desired_zoom_level (0x320/0x321),
//   .unknown_348 (0x348). Identical tail to unit_validate_and_clear_weapon_switch (0x5659c0).
// register convention: unit pointer carried over in an unresolved register (unaff_EBX) and a
//   precomputed "should notify" flag in the zero flag (in_ZF), i.e. this is reached as a shared
//   tail rather than called with its own fresh arguments.
//   // blam-cc: EBX -> unit (already resolved by the caller), in_ZF -> skip_notify, EDX -> sound_definition_index
// UNSURE: this entry point's real callers, and therefore what unit pointer and flag it actually
//   receives, are outside this batch; modelled as explicit parameters for compilability.
// FIXED (register inputs, objdump): EDX (read at 0x565a77, the call to sound_start_unspatialized)
// is a genuine pass-through input -- sound_start_unspatialized.c's own recovered convention is
// EDX -> definition_index, and this function never writes EDX before forwarding to it, so it
// must be supplied by this function's own caller too. Added as sound_definition_index and
// forwarded; the extern's signature (previously a placeholder `float amount`) is corrected to
// match sound_start_unspatialized.c's real one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, src/sound/sound_start_unspatialized.c
extern void unit_invalidate_local_player_zoom_level(void);         // 0x4726f0, UNSURE: no traced args

void unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index) // blam-cc: see file header
{
    if (!skip_notify) {
        sound_start_unspatialized(sound_definition_index, 1.0f);
    }
    unit->zoom_level = -1;
    unit->desired_zoom_level = -1;
    unit->unknown_348 = 0.0f;
    unit_invalidate_local_player_zoom_level();
}

#if 0
Original Ghidra decompilation (0x565a70):

void unit_update(void)

{
  int unaff_EBX;
  bool in_ZF;

  if (!in_ZF) {
    FUN_00543dd0(0x3f800000);
  }
  *(undefined1 *)(unaff_EBX + 800) = 0xff;
  *(undefined1 *)(unaff_EBX + 0x321) = 0xff;
  *(undefined4 *)(unaff_EBX + 0x348) = 0;
  FUN_004726f0();
  return;
}
#endif
