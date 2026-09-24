// unit_reset_light_effect  (Ghidra: FUN_0056ec10; renamed from the phase2 proposal)
// address 0x56ec10, size 72 bytes
// name confidence: 0.25 (phase2 proposal "unit_reset_light_effect" at 0.25, kept for lack of a
//   better candidate)
// rewrite confidence: 0.3
// evidence: callee sound_start_at_object_marker established elsewhere in this module (0x543ce0, see
//   src/units/unit_update_animation_timers.c) as a "set effect/sound intensity" helper; the
//   (effect, 0, 1.0, 0) argument pattern matches the "trigger at full intensity" call sites in
//   0x56f210 and 0x574f30.
// register convention: an effect/state handle in ECX (in_ECX); the return value is whatever
//   animation_state_advance returns.
//   // blam-cc: ECX -> effect_index
// UNSURE: animation_state_advance's purpose and its literal argument (1) are not resolved anywhere in this
//   module; kept as an opaque call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern uint16_t animation_state_advance(uint32_t flag); // 0x4d48d0, UNSURE
extern datum_index sound_start_at_object_marker(datum_index effect_index, void *position, float intensity,
                                 uint32_t flag); // 0x543ce0, UNSURE signature

// Performs an unresolved state reset (animation_state_advance) and, if a valid effect handle is provided,
// resets its intensity to full via sound_start_at_object_marker.
uint16_t unit_reset_light_effect(datum_index effect_index)
{
    uint16_t result = animation_state_advance(1);

    if (effect_index != k_datum_index_none) {
        sound_start_at_object_marker(effect_index, 0, 1.0f, 0);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x56ec10):

undefined2 FUN_0056ec10(void)

{
  undefined2 uVar1;
  int in_ECX;

  uVar1 = FUN_004d48d0(1);
  if (in_ECX != -1) {
    FUN_00543ce0(in_ECX,0,0x3f800000,0);
  }
  return uVar1;
}
#endif
