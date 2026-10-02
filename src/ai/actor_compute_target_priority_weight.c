// actor_compute_target_priority_weight  (Ghidra: actor_compute_target_priority_weight, renamed)
// address 0x414590, size 301 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x414590..0x4146bc (all constants).)
// evidence: phase-4 summary "computes a numeric priority weight for how attractive a
// potential threat/target is, combining class, distance, occupancy and relationship
// factors"; its only caller (0x41d75f, inside the recognition-refresh routine around
// ai_target_distance_qsort_compare) stores the result into prop.unknown_54 right after
// storing actor_rate_potential_target's result into prop.desirability (+0x50), so this is
// a companion score next to the main target desirability.
// register convention: prop_index in EAX (Ghidra's in_EAX), actor_index in ECX
// (Ghidra's in_ECX).
// blam-cc: EAX -> prop_index, ECX -> actor_index
// UNSURE: the float10 (x87 80-bit) return in Ghidra is just the x87 calling convention for
// a float result; rewritten as float.
// UNSURE: prop.unknown_76, .unknown_121, .unknown_123 and .unknown_12f have no established
// meaning beyond this function's own use of them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> prop_index, ECX -> actor_index
float actor_compute_target_priority_weight(datum_index prop_index, datum_index actor_index)
{
    prop *p;
    actor *self;
    float weight;
    float occupancy_bonus;
    float relationship_scale;
    datum_index active_unit;

    p = &((prop *)prop_data->data)[prop_index & 0xffff];
    weight = 0.0f;

    if (p->state < 2 || 3 < p->state) {
        if (p->enemy != 0 && 3 < p->state && p->state < 6) {
            weight = 1.5f;
        }
    } else if (p->dead == 0) {
        weight = (p->enemy == 0) ? 1.0f : 2.0f;
    } else if (p->dead_ticks < 0xd2) {
        weight = 1.8f;
    } else {
        weight = 0.4f;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    active_unit = self->active_unit_index;
    if (p->object_index == active_unit || (datum_index)p->relationship_object_index == active_unit) {
        weight = 0.0f;
    }

    relationship_scale = (p->relationship_object_index == -1) ? 1.0f : 1.5f;

    switch (p->speed_class) {
    case 1:
        occupancy_bonus = 0.5f * relationship_scale;
        weight += occupancy_bonus;
        break;
    case 2:
        occupancy_bonus = relationship_scale;
        weight += occupancy_bonus;
        break;
    case 3:
        occupancy_bonus = relationship_scale + relationship_scale;
        weight += occupancy_bonus;
        break;
    default:
        // no occupancy_bonus term added
        break;
    }

    if (p->shooting != 0) {
        weight = relationship_scale + relationship_scale + weight;
    }

    switch (p->distance_class) {
    case 1:
        weight *= 0.6f;
        break;
    case 3:
        return weight * 0.4f;
    case 4:
        return weight * 0.2f;
    default:
        break;
    }
    return weight;
}

#if 0
Original Ghidra decompilation (0x414590):

float10 FUN_00414590(void)

{
  char cVar1;
  short sVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  uint in_ECX;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;

  fVar5 = (float10)0.0;
  iVar4 = (in_EAX & 0xffff) * 0x138;
  sVar2 = *(short *)(iVar4 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
  iVar4 = iVar4 + *(int *)(DAT_008802c0 + 0x34);
  if ((sVar2 < 2) || (3 < sVar2)) {
    if ((*(char *)(iVar4 + 0x60) != '\0') && ((3 < sVar2 && (sVar2 < 6)))) {
      fVar5 = (float10)1.5;
    }
  }
  else if (*(char *)(iVar4 + 0x127) == '\0') {
    if (*(char *)(iVar4 + 0x60) == '\0') {
      fVar5 = (float10)1.0;
    }
    else {
      fVar5 = (float10)2.0;
    }
  }
  else if (*(short *)(iVar4 + 0x76) < 0xd2) {
    fVar5 = (float10)1.8;
  }
  else {
    fVar5 = (float10)0.4;
  }
  iVar3 = *(int *)((in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x158);
  if ((*(int *)(iVar4 + 0x18) == iVar3) || (*(int *)(iVar4 + 0x110) == iVar3)) {
    fVar5 = (float10)0.0;
  }
  if (*(int *)(iVar4 + 0x110) == -1) {
    fVar6 = (float10)1.0;
  }
  else {
    fVar6 = (float10)1.5;
  }
  cVar1 = *(char *)(iVar4 + 0x123);
  if (cVar1 == '\x01') {
    fVar7 = (float10)0.5 * fVar6;
  }
  else {
    fVar7 = fVar6;
    if (cVar1 != '\x02') {
      if (cVar1 != '\x03') goto LAB_00414684;
      fVar7 = fVar6 + fVar6;
    }
  }
  fVar5 = fVar7 + fVar5;
LAB_00414684:
  if (*(char *)(iVar4 + 0x12f) != '\0') {
    fVar5 = fVar6 + fVar6 + fVar5;
  }
  cVar1 = *(char *)(iVar4 + 0x121);
  if (cVar1 == '\x01') {
    fVar5 = fVar5 * (float10)0.6;
  }
  else {
    if (cVar1 == '\x03') {
      return fVar5 * (float10)0.4;
    }
    if (cVar1 == '\x04') {
      return fVar5 * (float10)0.2;
    }
  }
  return fVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
