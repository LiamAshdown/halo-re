// actor_get_cached_wander_position  (Ghidra: actor_get_cached_wander_position, renamed)
// address 0x4281f0, size 126 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: types/ai.h actor.target_combat_status(0x268)/facing_unknown_180(0x180)/
//   unknown_6a0/unknown_60c; actor_mode_definitions[16] (0x00655254, combat_grade at +0x04)
//   already established elsewhere in this module. Phase-4 summary: "Writes one of two cached
//   3-float positions into the output vector supplied via EDX, selected by internal actor
//   state flags; returns whether a position was produced."
//   UNSURE: actor.unknown_63c (a 16-byte unnamed run in types/ai.h) is read here as a
//   real_vector3d; kept as a raw offset rather than asserting a name for it.
// register convention: EAX -> actor_index, EDX -> out_position (real_vector3d*).
//   // blam-cc: EAX -> actor_index, EDX -> out_position

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

// blam-cc: EAX -> actor_index, EDX -> out_position
// For an actor deep enough into combat (target_combat_status > 8) whose current mode has
// combat_grade 4, writes one of two cached vectors into *out_position: facing_unknown_180
// when unknown_6a0 is set, otherwise the unnamed vector at unknown_63c when unknown_60c is
// positive. Returns whether a vector was written.
uint8_t actor_get_cached_wander_position(datum_index actor_index, real_vector3d *out_position)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_combat_status > 8 && actor_mode_definitions[self->mode].combat_grade == 4) {
        if (self->grenade_throw_pending != 0) {
            *out_position = self->facing_unknown_180;
            return 1;
        }
        if (self->firing_target_type > 0) {
            *out_position = *(real_vector3d *)&self->target_aim_vector[0]; // UNSURE offset
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4281f0):

uint FUN_004281f0(void)

{
  uint in_EAX;
  uint uVar1;
  uint uVar2;
  undefined3 uVar3;
  int iVar4;
  undefined4 *in_EDX;

  uVar1 = (in_EAX & 0xffff) * 0x724;
  iVar4 = *(int *)(DAT_00880360 + 0x34) + uVar1;
  uVar2 = uVar1 & 0xffffff00;
  if ((8 < *(short *)(iVar4 + 0x268)) &&
     (*(short *)(&DAT_00655258 + *(short *)(iVar4 + 0x6c) * 0x38) == 4)) {
    uVar3 = (undefined3)(uVar1 >> 8);
    if (*(char *)(iVar4 + 0x6a0) != '\0') {
      *in_EDX = *(undefined4 *)(iVar4 + 0x180);
      in_EDX[1] = *(undefined4 *)(iVar4 + 0x184);
      in_EDX[2] = *(undefined4 *)(iVar4 + 0x188);
      return CONCAT31(uVar3,1);
    }
    if (0 < *(short *)(iVar4 + 0x60c)) {
      *in_EDX = *(undefined4 *)(iVar4 + 0x63c);
      in_EDX[1] = *(undefined4 *)(iVar4 + 0x640);
      uVar2 = CONCAT31(uVar3,1);
      in_EDX[2] = *(undefined4 *)(iVar4 + 0x644);
    }
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
