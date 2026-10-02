// actor_check_weapon_pickup_reachable  (Ghidra: actor_check_weapon_pickup_reachable, renamed)
// address 0x4041d0, size 296 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4041d0..0x4042f7; offsets probed)
// evidence: types/ai.h actor.unit_index (0x18)/encounter_index (0x34)/active_unit_index
//   (0x158); prop.kind (0x24)/relationship_object_index (0x110)/last_known_position (0xbc);
//   types/tags.h Scenario.encounters (pointer at 0x430), ScenarioEncounter.firing_positions
//   (pointer at 0x9c, stride 0x18); phase-4 summary "checks whether the actor can path to
//   and pick up a weapon held by another unit, copying the target position into the record
//   on success".
// register convention: actor index in EAX, an aim/look record pointer as the recognized
//   stack parameter.
//   // blam-cc: EAX -> actor_index, stack -> record
// TYPES-GAP: `record` matches the same per-mode aim-state offsets (+0x1c input prop index,
//   +0x20/+0x24/+0x28/+0x2c output flag+point) used elsewhere in this session
//   (actor_squad_action_execute.c's aim_state); not backed by a shared struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern Scenario *global_scenario; // 0x00746f8c

extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point,
    uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator); // 0x569190, stack, EAX accumulator
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying); // 0x42b270, AX, CX, ESI, EDI, stack

uint8_t actor_check_weapon_pickup_reachable(uint32_t actor_index, uint8_t *record)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index target_prop_index = *(datum_index *)(record + 0x1c);
    int16_t firing_position_index = *(int16_t *)(record + 8);
    uint8_t result = 0;

    if (target_prop_index == (datum_index)k_datum_index_none || a->encounter_index == (datum_index)k_datum_index_none || firing_position_index == -1) {
        return 0;
    }

    {
        prop *p = &((prop *)prop_data->data)[target_prop_index & 0xffff];
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioFiringPosition *positions = (ScenarioFiringPosition *)encounters[a->encounter_index & 0xffff].firing_positions.pointer;
        int16_t status;
        real_point3d self_position;

        // 0x404258..0x40429b: the firing position seen from the unit's marker (EAX = the local) against the prop's
        // cluster (+0x100) and position (+0x104)
        unit_add_marker_relative_offset(a->unit_index, 2, (float *)&positions[firing_position_index], 0, 0, &self_position);
        status = (int16_t)actor_evaluate_engagement_reachability(
            *(int16_t *)((uint8_t *)&positions[firing_position_index] + 0xe), p->cluster_index,
            (real_point3d *)&p->head_position_x, &self_position, 1, 0, p->relationship_object_index,
            a->active_unit_index != (datum_index)k_datum_index_none);

        if (p->state > 1 && p->state < 4) {
            result = (status == 0);
        }
        if (status == 0 || status == 3) {
            record[0x20] = 1;
            *(real_point3d *)(record + 0x24) = p->last_known_position;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4041d0):

bool FUN_004041d0(int param_1)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  bool bVar4;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  bVar4 = false;
  if (((*(uint *)(param_1 + 0x1c) != 0xffffffff) && (*(uint *)(iVar2 + 0x34) != 0xffffffff)) &&
     (*(short *)(param_1 + 8) != -1)) {
    iVar3 = (*(uint *)(param_1 + 0x1c) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    FUN_00569190(*(undefined4 *)(iVar2 + 0x18),2,
                 *(int *)((*(uint *)(iVar2 + 0x34) & 0xffff) * 0xb0 + 0x9c +
                         *(int *)(global_scenario + 0x430)) + *(short *)(param_1 + 8) * 0x18,0,0);
    sVar1 = FUN_0042b270(1,0,*(undefined4 *)(iVar3 + 0x110),*(int *)(iVar2 + 0x158) != -1);
    if ((1 < *(short *)(iVar3 + 0x24)) && (*(short *)(iVar3 + 0x24) < 4)) {
      bVar4 = sVar1 == 0;
    }
    if ((sVar1 == 0) || (sVar1 == 3)) {
      *(undefined1 *)(param_1 + 0x20) = 1;
      *(undefined4 *)(param_1 + 0x24) = *(undefined4 *)(iVar3 + 0xbc);
      *(undefined4 *)(param_1 + 0x28) = *(undefined4 *)(iVar3 + 0xc0);
      *(undefined4 *)(param_1 + 0x2c) = *(undefined4 *)(iVar3 + 0xc4);
    }
    return bVar4;
  }
  return false;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
