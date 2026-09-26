// actor_place_new_unit  (Ghidra: actor_place_new_unit, already named)
// address 0x427080, size 500 bytes
// name confidence: 0.55   rewrite confidence: 0.15
// evidence: phase-4 summary "Creates and places a new AI-controlled unit object at a
// starting-location/encounter placement, applying its AI properties and binding an actor to
// it." types/objects.h object_placement_data (established). Calls
// actor_new_and_attach_to_unit (0x426ac0, already rewritten in this module),
// actor_apply_unit_definition_properties (0x426cf0, already rewritten in this module),
// object_placement_data_initialize / object_new_with_datum_role_control (both established),
// object_delete_recursive / object_delete_unparented, and objects_garbage_collection.
//   UNSURE: this is one of the least-confident rewrites in this pass. The Scenario
//   starting-location/encounter-placement structures this reaches into (ScenarioSquad's
//   placement block at +0x24/+0x26/+0x20, and the ActorVariant fields read via the local
//   caller-owned real_point3d+yaw+flags block Ghidra shows only as `in_EAX`) are not modeled
//   with named types here; kept as raw offsets. The mapping of this function's own five
//   arguments onto actor_new_and_attach_to_unit's twelve is best-effort, following that
//   function's already-established parameter order, not independently confirmed with
//   objdump for this specific call site.
// register convention: stack -> actor_variant_or_palette_tag, encounter_index, squad_index,
//   use_palette_entry, unit_type_index; EAX -> placement (a caller-owned position/yaw/flags
//   record).
//   // blam-cc: EAX -> placement_request, stack -> actor_variant_or_palette_tag, encounter_index,
//   //   squad_index, use_palette_entry, unit_type_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *object_data;     // 0x008603b0
extern Scenario *global_scenario;       // 0x00746f8c
extern int32_t map_difficulty_or_kind; // 0x00719720, UNSURE name
extern void **object_type_role_table;  // 0x0069bfdc

extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN
extern void objects_garbage_collection(void); // 0x4f9c60
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_delete_recursive(datum_index object_index, uint32_t flag); // 0x4f59d0, UNSURE signature
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0, UNSURE signature
extern void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index); // 0x426cf0
extern datum_index actor_new_and_attach_to_unit(
    char reuse_existing, datum_index unit_index, datum_index actor_variant_tag,
    uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor,
    char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68); // 0x426ac0

// The caller-owned position/yaw/flags record this function reads via EAX; only the fields it
// itself uses are named.

// blam-cc: EAX -> placement_request, stack -> actor_variant_or_palette_tag, encounter_index,
//   squad_index, use_palette_entry, unit_type_index
// Creates and places a new AI-controlled unit object at a starting-location/encounter
// placement, applying its AI-related unit-definition properties and binding an actor to it
// (or reusing a compatible existing one, for swarm-type actors), cleaning the object back up
// again on failure.
datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index,
                                 int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index,
                                 const actor_placement_request *placement_request)
{
    datum_index actor_variant_tag = actor_variant_or_palette_tag;
    const uint32_t *actor_variant;
    object_placement_data placement;
    uint32_t role;
    datum_index unit_index;
    datum_index result;

    objects_garbage_collection();

    actor_variant = (const uint32_t *)(tag_instances[actor_variant_tag & 0xffff].data);
    if (use_palette_entry != 0) {
        actor_variant_tag = (datum_index)actor_variant[0xc]; // UNSURE offset: an ActorPalette-style indirection
        actor_variant = (const uint32_t *)(tag_instances[actor_variant_tag & 0xffff].data);
    }

    {
        const uint32_t *actor_tag = (const uint32_t *)(tag_instances[actor_variant[4] & 0xffff].data); // ActorVariant.actor_definition
        (void)actor_tag;
    }

    object_placement_data_initialize(&placement, (datum_index)actor_variant[8], (datum_index)k_datum_index_none); // UNSURE offset: ActorVariant.unit tag
    placement.position = placement_request->position;
    placement.forward.i = (float)cos((double)placement_request->yaw);
    placement.forward.j = (float)sin((double)placement_request->yaw);

    role = 3;
    if (map_difficulty_or_kind == 2 &&
        *(int32_t *)((uint8_t *)object_type_role_table[*(uint16_t *)&placement.definition_tag] + 0x10) != -1) { // UNSURE
        role = 0;
    }

    unit_index = object_new_with_datum_role_control(&placement, role);
    if (unit_index == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    {
        char reuse_existing = (*(const uint32_t *)(tag_instances[actor_variant[4] & 0xffff].data) >> 0x1a) & 1; // Actor.flags bit 26 "swarm"
        char start_active = 0;
        uint16_t unknown_60 = 0;
        int16_t unknown_62 = 0;

        actor_apply_unit_definition_properties(actor_variant_tag, unit_index);

        if (encounter_index != (datum_index)k_datum_index_none) {
            // Raw offsets, kept exactly as decompiled: Scenario+0x430 is the encounters
            // TagReflexive's pointer field; ScenarioEncounter is 0xb0 (+0x84 the squads
            // TagReflexive's own pointer, +0x20 ScenarioEncounter.flags); ScenarioSquad is
            // 0xe8 (+0x24 initial_state, +0x26 return_state).
            const uint8_t *encounters_base = *(const uint8_t **)((const uint8_t *)global_scenario + 0x430);
            const uint8_t *scenario_encounter = encounters_base + (encounter_index & 0xffff) * 0xb0;
            const uint8_t *squads_base = *(const uint8_t **)(scenario_encounter + 0x84);
            const uint8_t *squad = squads_base + squad_index * 0xe8;

            unknown_60 = *(const uint16_t *)(squad + 0x24);
            unknown_62 = *(const int16_t *)(squad + 0x26);
            start_active = (char)((*(const uint32_t *)(scenario_encounter + 0x20) >> 4) & 1); // UNSURE bit
        }

        if (placement_request->unknown_16 > 0) {
            unknown_60 = (uint16_t)placement_request->unknown_16;
        }
        if (placement_request->unknown_1c > 0) {
            unknown_62 = placement_request->unknown_1c;
        }

        result = actor_new_and_attach_to_unit(reuse_existing, unit_index, actor_variant_tag,
                                              encounter_index, squad_index, 0, (datum_index)k_datum_index_none,
                                              start_active, unknown_60, unknown_62,
                                              *(const uint16_t *)&placement_request->unknown_1a,
                                              (uint8_t)placement_request->unknown_12);
    }

    if (result == (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        int32_t network_role = unit_object->network_role;
        if (network_role == 0) {
            object_delete_unparented(unit_index);
        } else if (network_role != 3) {
            return (datum_index)k_datum_index_none;
        }
        object_delete_recursive(unit_index, 0);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x427080):

int actor_place_new_unit(uint param_1,uint param_2,int param_3,char param_4,undefined2 param_5)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  undefined4 *in_EAX;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  float10 fVar10;
  uint local_9c;
  uint local_94;
  uint local_90 [5];
  undefined2 local_7a;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  float local_5c;
  float local_58;
  undefined4 local_54;

  objects_garbage_collection();
  iVar2 = DAT_0087bc14;
  iVar7 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_4 != '\0') {
    param_1 = *(uint *)(iVar7 + 0x30);
    iVar7 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  puVar1 = *(undefined4 **)((*(uint *)(iVar7 + 0x10) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  object_placement_data_initialize(*(undefined4 *)(iVar7 + 0x20),0xffffffff);
  fVar10 = (float10)fcos((float10)(float)in_EAX[3]);
  local_78 = *in_EAX;
  local_74 = in_EAX[1];
  local_70 = in_EAX[2];
  local_54 = 0;
  local_7a = param_5;
  uVar4 = 3;
  local_5c = (float)fVar10;
  fVar10 = (float10)fsin((float10)(float)in_EAX[3]);
  local_58 = (float)fVar10;
  if ((DAT_00719720 == 2) &&
     (*(int *)((&PTR_PTR_0069bfdc)[**(short **)((local_90[0] & 0xffff) * 0x20 + 0x14 + iVar2)] +
              0x10) != -1)) {
    uVar4 = 0;
  }
  uVar5 = object_new_with_datum_role_control(local_90,uVar4);
  if (uVar5 != 0xffffffff) {
    local_94 = CONCAT31(local_94._1_3_,(byte)((uint)*puVar1 >> 0x1a)) & 0xffffff01;
    uVar3 = local_9c >> 8;
    local_9c = local_9c & 0xffffff00;
    uVar8 = 0;
    uVar9 = 0;
    FUN_00426cf0(uVar5);
    if (param_2 != 0xffffffff) {
      iVar7 = *(int *)(global_scenario + 0x430);
      iVar6 = (param_2 & 0xffff) * 0xb0;
      iVar2 = *(int *)(iVar6 + 0x84 + iVar7);
      uVar8 = CONCAT22((short)((uint)iVar7 >> 0x10),*(undefined2 *)(param_3 * 0xe8 + 0x24 + iVar2));
      uVar9 = CONCAT22((short)((uint)iVar2 >> 0x10),*(undefined2 *)(param_3 * 0xe8 + iVar2 + 0x26));
      local_9c = CONCAT31((int3)uVar3,(char)(*(uint *)(iVar6 + iVar7 + 0x20) >> 4)) & 0xffffff01;
    }
    if (0 < (short)*(ushort *)((int)in_EAX + 0x16)) {
      uVar8 = (uint)*(ushort *)((int)in_EAX + 0x16);
    }
    if (0 < (short)*(ushort *)(in_EAX + 5)) {
      uVar9 = (uint)*(ushort *)(in_EAX + 5);
    }
    iVar7 = actor_new_and_attach_to_unit
                      (local_94,uVar5,param_1,param_2,param_3,0,0xffffffff,local_9c,uVar8,uVar9,
                       *(undefined2 *)((int)in_EAX + 0x1a),(short)*(char *)((int)in_EAX + 0x12));
    if (iVar7 == -1) {
      iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) + 4);
      if (iVar2 == 0) {
        object_delete_unparented();
      }
      else if (iVar2 != 3) {
        return -1;
      }
      object_delete_recursive(uVar5,0);
    }
    return iVar7;
  }
  return -1;
}
#endif
