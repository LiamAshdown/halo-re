// actor_target_hearing_check  (Ghidra: actor_target_hearing_check; named from out/phase2/results/ai_02.json)
// address 0x41c030, size 429 bytes
// name confidence: 0.35   rewrite confidence: 0.9
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
#include "cache.h"
#include "objects.h"
#include "fn_ai.h"

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which
// Ghidra renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double sqrt(double x); // FSQRT
static float sqrt_f(float x) { return (float)sqrt((double)x); }

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

extern uint8_t scenario_location_background_sound_is_deafening_to_ais(bsp_leaf_reference *location); // 0x53e810, EAX
extern uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b,
    ScenarioStructureBSP *structure_bsp); // 0x552210, EAX, ECX, EDI

// REWRITTEN from objdump 0x41c030..0x41c1dc. EAX: actor; ECX: the listener block (+0x0 position, +0x18 facing, +0x24
//   location, +0x28 cluster word); EBX: the sound's gate (0 = silent); ESI: the sound position; stack: the sound's
//   location (cluster at +0x4) and stance. The Actor tag's hearing distance (+0x4c) is scaled by 0.8 for a sound
//   behind the listener, 0.7 / 0.4 by awareness 2 / 1, 0.2 / 0.45 / 0.7 for gates 4 / 1 / 3, 0.25 when either
//   location is deafening, 0.7 for a stance past 1. Heard (2, or 3 for gate >= 3) when inside that range and inside
//   the clusters' sound distance (PAS byte * 2.0157 * 2, at least the straight distance). The draft called both
//   helpers without operands.
// blam-cc: EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI -> listener_position,
//   stack -> record, stance
uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref,
    int16_t gate, real_point3d *listener_position)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *listener = (uint8_t *)target_ref;                                    // ecx
    int16_t listener_cluster;                                                     // [esp+0xc]
    int16_t source_cluster;                                                       // [esp+0x10]
    float range;
    float dx, dy, dz, distance_squared;
    uint8_t pas;
    float sound_distance;
    float distance;

    if (gate == 0) {
        return 0;
    }
    listener_cluster = *(int16_t *)(listener + 0x28);
    if (listener_cluster == -1) {
        return 0;
    }
    source_cluster = *(int16_t *)((uint8_t *)record + 0x4);
    if (source_cluster == -1) {
        return 0;
    }
    range = *(float *)((uint8_t *)tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data + 0x4c);
    dx = listener_position->x - *(float *)(listener + 0x0);
    dy = listener_position->y - *(float *)(listener + 0x4);
    dz = listener_position->z - *(float *)(listener + 0x8);
    distance_squared = dz * dz + dy * dy + dx * dx;
    if (!(dz * *(float *)(listener + 0x20) + dy * *(float *)(listener + 0x1c) + dx * *(float *)(listener + 0x18) >= 0.0f)) {
        range = range * 0.8f;
    }
    if (((actor *)a)->awareness_level == 2) {
        range = range * 0.7f;
    } else if (((actor *)a)->awareness_level == 1) {
        range = range * 0.4f;
    }
    if (gate == 4) {
        range = range * 0.2f;
    } else if (gate == 1) {
        range = range * 0.45f;
    } else if (gate == 3) {
        range = range * 0.7f;
    }
    if (scenario_location_background_sound_is_deafening_to_ais((bsp_leaf_reference *)(listener + 0x24)) ||
        scenario_location_background_sound_is_deafening_to_ais((bsp_leaf_reference *)record)) {
        range = range * 0.25f;
    }
    if (stance != 0 && stance != 1) {
        range = range * 0.7f;
    }
    if (!(range * range > distance_squared)) {
        return 0;
    }
    pas = cluster_sound_distance_lookup(listener_cluster, source_cluster, global_structure_bsp);
    if (pas & 0x80) {
        return 0;
    }
    sound_distance = (float)(int32_t)(pas & 0x7f) * 2.0157480f;
    sound_distance = sound_distance + sound_distance;
    distance = (float)sqrt((double)distance_squared);
    if (!(sound_distance > distance)) {
        sound_distance = distance;
    }
    if (sound_distance >= range) {
        return 0;
    }
    return (uint16_t)((gate >= 3) + 2);
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
