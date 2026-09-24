// actor_target_hearing_check  (Ghidra: actor_target_hearing_check; named from out/phase2/results/ai_02.json)
// address 0x41c030, size 429 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase2/results/ai_02.json -- computes squared planar (really full 3D here)
//   distance to a target and compares it against a stealth/crouch-scaled hearing radius, then
//   calls cluster_sound_distance_lookup (0x552210, named from this batch's callee list; phase2
//   called it chimera__cluster_sound_distance_func) to test an actual acoustic path, returning
//   a graded detection level (0, 2 or 3 on the paths this rewrite can follow).
// register convention: EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI ->
//   listener_position; param_1 (record with an int16 at +4) and param_2 (int16 stance) are
//   Ghidra's recognized stack parameters.
//   // blam-cc: EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI -> listener_position,
//   //   stack -> record, stance
//
// UNSURE, heavily: this function's decompilation is dominated by x87 FPU-stack leftovers
// (`extraoutSTxx`) that Ghidra could not tie back to real values, and both calls to scenario_location_background_sound_is_deafening_to_ais
// appear to return a bool in EAX *and* a float in ST0 simultaneously, which plain C cannot
// model as one call. This rewrite represents that with a small return struct
// (bool_float_return) purely to keep both observed outputs, and equates the final
// `extraout_ST0_01` comparison with the already-computed effective hearing radius (the most
// coherent reading: reject the candidate before the acoustic-path lookup on radius, then accept
// it only if the *actual* acoustic path distance is still under that same radius). Every
// pointer parameter is left untyped (raw offsets) since neither record's real struct is
// identified. actor_index (EAX) is read here only to size the actor lookup Ghidra performed;
// it is otherwise unused, which is itself suspicious for a "hearing" check and is not resolved
// further.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which
// Ghidra renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double sqrt(double x); // FSQRT
static float sqrt_f(float x) { return (float)sqrt((double)x); }

extern data_array *actor_data; // 0x00880360

// UNSURE: see file header -- scenario_location_background_sound_is_deafening_to_ais returns both a bool (AL) and a float (ST0);
// that shape is types/ai.h bool_float_return, shared with actor_rate_potential_target.c.
extern bool_float_return scenario_location_background_sound_is_deafening_to_ais(void); // 0x53e810, UNSURE signature, no args recovered
extern int32_t cluster_sound_distance_lookup(void); // 0x552210, UNSURE signature, no args recovered

// blam-cc: EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI -> listener_position,
//   stack -> record, stance
// Determines whether the actor can hear a nearby target based on distance and acoustic cluster
// propagation, returning a graded detection level.
uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
                                    void *target_ref, int16_t gate, real_point3d *listener_position)
{
    float dx, dy, dz;
    float distance_squared;
    float radius;
    bool_float_return r;
    int32_t lookup;
    float acoustic_distance;
    float actual_distance;

    (void)actor_index; // read by Ghidra only to compute an unused actor base pointer

    if (gate == 0 || *(int16_t *)((uint8_t *)target_ref + 40) == -1 ||
        *(int16_t *)((uint8_t *)record + 4) == -1) {
        return 0;
    }

    dx = listener_position->x - ((real_point3d *)target_ref)->x;
    dy = listener_position->y - ((real_point3d *)target_ref)->y;
    dz = listener_position->z - ((real_point3d *)target_ref)->z;
    distance_squared = dz * dz + dy * dy + dx * dx;

    r = scenario_location_background_sound_is_deafening_to_ais();
    radius = r.value;
    if (r.truthy == 0) {
        r = scenario_location_background_sound_is_deafening_to_ais();
        radius = r.value;
    }
    if (r.truthy != 0) {
        radius = radius * 0.25f;
    }

    if (stance != 0 && stance != 1) {
        radius = radius * 0.7f;
    }

    if (distance_squared < radius * radius) {
        lookup = cluster_sound_distance_lookup();
        if (-1 < (int8_t)lookup) {
            acoustic_distance = (float)(lookup & 0x7f) * 2.015748f;
            acoustic_distance = acoustic_distance + acoustic_distance;
            actual_distance = sqrt_f(distance_squared);
            if (acoustic_distance <= actual_distance) {
                acoustic_distance = actual_distance;
            }
            if (acoustic_distance < radius) {
                return (uint16_t)((2 < gate) + 2);
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x41c030):

uint FUN_0041c030(int param_1,short param_2)

{
  float fVar1;
  uint in_EAX;
  uint uVar2;
  float *in_ECX;
  short unaff_BX;
  float *unaff_ESI;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float10 fVar3;
  float10 extraout_ST0_01;

  uVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (((unaff_BX != 0) && (uVar2 = 0, *(short *)(in_ECX + 10) != -1)) &&
     (uVar2 = (uint)*(short *)(param_1 + 4), *(short *)(param_1 + 4) != -1)) {
    fVar1 = (*unaff_ESI - *in_ECX) * (*unaff_ESI - *in_ECX) +
            (unaff_ESI[1] - in_ECX[1]) * (unaff_ESI[1] - in_ECX[1]) +
            (unaff_ESI[2] - in_ECX[2]) * (unaff_ESI[2] - in_ECX[2]);
    uVar2 = FUN_0053e810();
    fVar3 = extraout_ST0;
    if (((char)uVar2 != '\0') ||
       (uVar2 = FUN_0053e810(), fVar3 = extraout_ST0_00, (char)uVar2 != '\0')) {
      fVar3 = fVar3 * (float10)0.25;
    }
    if ((param_2 != 0) && (param_2 != 1)) {
      fVar3 = fVar3 * (float10)0.7;
    }
    uVar2 = uVar2 & 0xffff0000;
    if (((float10)fVar1 < fVar3 * fVar3) &&
       (uVar2 = cluster_sound_distance_lookup(), -1 < (char)uVar2)) {
      fVar3 = (float10)(uVar2 & 0x7f) * (float10)2.015748;
      fVar3 = fVar3 + fVar3;
      fVar1 = SQRT(fVar1);
      if (fVar3 <= (float10)fVar1) {
        fVar3 = (float10)fVar1;
      }
      uVar2 = uVar2 & 0xffff0000;
      if (fVar3 < extraout_ST0_01) {
        return (2 < unaff_BX) + 2;
      }
    }
  }
  return uVar2 & 0xffff0000;
}
#endif
