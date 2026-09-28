// actor_is_target_within_engagement_range  (Ghidra: actor_is_target_within_engagement_range, renamed)
// address 0x403dc0, size 305 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x403dc0..0x403ef0; offsets probed)
// evidence: types/ai.h actor.encounter_index (0x34)/firing_position_index (0x3b8)/
//   active_movement (0x46c)/movement_action_complete (0x4a8)/movement_completed (0x484);
//   types/tags.h Scenario.encounters (pointer at 0x430), ScenarioEncounter.firing_positions
//   (pointer at 0x9c, stride 0x18); phase-4 summary "whether the actor still perceives its
//   last known target position as within engagement range, accounting for a leading
//   squadmate's status".
// register convention: actor index in EAX, the sole real parameter.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor.active_movement+4 (its destination field, a real_point3d per types/ai.h) is
//   read here as a plain int16 firing-position id instead, which is consistent with that
//   field being a per-kind union (as the header already notes) when active_movement.type==3.
//   actor+0xb8 falls inside actor.mode_data.raw; read here as a prop_data index whose prop+0x38
//   (an already-named but undecoded field) selects a squadmate status.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern Scenario *global_scenario; // 0x00746f8c

extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, not yet rewritten (an engagement-range radius)

uint8_t actor_is_target_within_engagement_range(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    ScenarioFiringPosition *fp;
    float dx, dy, dz, range;
    datum_index leader_prop_index;

    if (a->encounter_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    if (a->firing_position_index == -1) {
        return 0;
    }

    {
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioFiringPosition *positions = (ScenarioFiringPosition *)encounters[a->encounter_index & 0xffff].firing_positions.pointer;
        fp = &positions[a->firing_position_index];
    }

    if ((a->movement_action_complete == 0 || a->movement_completed != 0) &&
        a->active_movement.type == 3 &&
        *(int16_t *)((uint8_t *)&a->active_movement + 4) == a->firing_position_index) {
        return 1;
    }

    range = actor_compute_accuracy_scale(actor_index);
    dx = fp->position.x - a->body_position.x;
    dy = fp->position.y - a->body_position.y;
    dz = fp->position.z - a->body_position.z;
    if (range * range <= dx * dx + dy * dy + dz * dz) {
        return 0;
    }

    leader_prop_index = *(datum_index *)(a->mode_data.raw + (0xb8 - 0x9c));
    if (leader_prop_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[leader_prop_index & 0xffff];
        if (p->unknown_38 != 0 && p->unknown_38 != 1) {
            return 1;
        }
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x403dc0):

bool FUN_00403dc0(void)

{
  int iVar1;
  float *pfVar2;
  short sVar3;
  bool bVar4;
  uint in_EAX;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;
  float10 fVar8;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(uint *)(iVar1 + 0x34) == 0xffffffff) {
    return false;
  }
  sVar3 = *(short *)(iVar1 + 0x3b8);
  if (sVar3 != -1) {
    pfVar2 = (float *)(*(int *)((*(uint *)(iVar1 + 0x34) & 0xffff) * 0xb0 + 0x9c +
                               *(int *)(global_scenario + 0x430)) + sVar3 * 0x18);
    if ((((*(char *)(iVar1 + 0x4a8) == '\0') || (*(char *)(iVar1 + 0x484) != '\0')) &&
        (*(short *)(iVar1 + 0x46c) == 3)) && (*(short *)(iVar1 + 0x470) == sVar3)) {
      return true;
    }
    fVar5 = (float10)FUN_00429620();
    fVar6 = (float10)*pfVar2 - (float10)*(float *)(iVar1 + 300);
    fVar7 = (float10)pfVar2[1] - (float10)*(float *)(iVar1 + 0x130);
    fVar8 = (float10)pfVar2[2] - (float10)*(float *)(iVar1 + 0x134);
    if (fVar5 * fVar5 <= fVar8 * fVar8 + fVar7 * fVar7 + fVar6 * fVar6) {
      return false;
    }
    bVar4 = false;
    if (*(uint *)(iVar1 + 0xb8) != 0xffffffff) {
      sVar3 = *(short *)((*(uint *)(iVar1 + 0xb8) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34)
                        + 0x38);
      if ((sVar3 != 0) && (sVar3 != 1)) {
        return true;
      }
      bVar4 = true;
    }
    return !bVar4;
  }
  return false;
}
#endif
