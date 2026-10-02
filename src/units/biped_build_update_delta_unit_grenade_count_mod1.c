// biped_build_update_delta_unit_grenade_count_mod1  (Ghidra: already carries this name, but
// out/phase4/units_types_notes.md flags it as unrelated/misleading -- "a real 153-byte impulse
// applier, but the entry address is mid-instruction and the name is unrelated". Kept as-is per
// this batch's naming rule: only still-FUN_ names are replaced.)
// address 0x55e9ff, size 153 bytes
// name confidence: 0.2 (name is known-wrong; no better one established)   rewrite confidence: 0.15
// evidence: object.angular_velocity at 0x08c (objects.h); unit_data.animation_state 0x2a3
//   (types/units.h). The tail (state_out selection from animation_state) is byte-for-byte the
//   same as biped_apply_idle_fidget's own tail, and this address falls squarely inside that
//   function's byte range reinterpreted mid-instruction -- almost certainly a Ghidra
//   mis-identified entry point rather than a genuine separate function.
// register convention: flags in EAX, a "no seat" boolean in BL, an output pointer in EBP, and
//   the unit's object base pointer directly (not an index) in ESI; param_1..param_6 are
//   Ghidra-recognized stack parameters, several of them (param_1, param_2) never read.
//   // blam-cc: EAX -> flags, BL -> already_idle, EBP -> state_out, ESI -> object_base,
//   //           stack -> (unused), (unused), magnitude, dir_x, dir_y, dir_z
// UNSURE: whether this is genuinely reachable code or purely a decompiler artifact of
//   overlapping instruction decoding (see out/phase4/units_types_notes.md).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double fcos(double x);
extern double fsin(double x);
extern real random_real_range(real min, real max); // 0x401050
extern void unit_rotate_basis_about_axis(uint32_t object_index); // 0x55e6b0, this batch

void biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base,
                                                        float magnitude, float dir_x, float dir_y,
                                                        float dir_z, char already_idle,
                                                        uint8_t *state_out)
{
    unit_data *unit = (unit_data *)((uint8_t *)object_base + k_unit_data_offset);

    if ((flags & 0x4100) != 0) {
        double angle = random_real_range(0.0, 6.2831855);
        dir_x = (float)fcos(angle);
        dir_y = (float)fsin(angle);
        dir_z = 0.0f;
    }
    object_base->angular_velocity.i += dir_x * magnitude;
    object_base->angular_velocity.j += dir_y * magnitude;
    object_base->angular_velocity.k += dir_z * magnitude;

    unit_rotate_basis_about_axis(0); // UNSURE: object_index, see file header (a mis-carved fragment of 0x55e940)

    {
        int8_t state = unit->animation_state;
        if (state == 0x27 || state == 0x28) {
            state_out[0] = 0x28;
        } else if (state == 0x14 || already_idle != 0) {
            state_out[0] = 0x14;
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e9ff):

void biped_build_update_delta_unit_grenade_count_mod1
               (undefined4 param_1,undefined4 param_2,float param_3,float param_4,float param_5,
               float param_6)

{
  char cVar1;
  uint in_EAX;
  char unaff_BL;
  undefined1 *unaff_EBP;
  int unaff_ESI;
  float10 fVar2;
  float fVar3;

  if ((in_EAX & 0x4100) != 0) {
    fVar3 = random_real_range(0.0,6.2831855);
    fVar2 = (float10)fcos((float10)fVar3);
    param_4 = (float)fVar2;
    fVar2 = (float10)fsin((float10)fVar3);
    param_5 = (float)fVar2;
    param_6 = 0.0;
  }
  *(float *)(unaff_ESI + 0x8c) = param_4 * param_3 + *(float *)(unaff_ESI + 0x8c);
  *(float *)(unaff_ESI + 0x90) = param_5 * param_3 + *(float *)(unaff_ESI + 0x90);
  *(float *)(unaff_ESI + 0x94) = param_6 * param_3 + *(float *)(unaff_ESI + 0x94);
  FUN_0055e6b0();
  cVar1 = *(char *)(unaff_ESI + 0x2a3);
  if ((cVar1 == '\'') || (cVar1 == '(')) {
    *unaff_EBP = 0x28;
  }
  else if ((cVar1 == '\x14') || (unaff_BL != '\0')) {
    *unaff_EBP = 0x14;
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
