// actor_place_new_unit  (Ghidra: actor_place_new_unit, already named)
// address 0x427080, size 500 bytes
// name confidence: 0.55   rewrite confidence: 0.95
// VERIFIED against disassembly 0x427080..0x427273 (2026-09-30); rewritten from it. EAX: the placement request (+0x0 position, +0xc yaw, +0x12 byte,
//   +0x14 / +0x16 state words, +0x1a word); stack: variant (or palette entry when use_palette_entry: its +0x30),
//   encounter, squad, use_palette_entry, permutation. Creates the variant's unit (+0x20) at the position facing
//   (cos yaw, sin yaw, 0) with role 3 (0 when [0x719720] == 2 and the unit's object type definition has a +0x10
//   entry), applies the variant's unit properties and attaches a new actor (0x426ac0) with the squad's initial /
//   return states (+0x24 / +0x26, overridden by the request's +0x16 / +0x14 when positive) and the encounter's
//   flag bit 4; on failure the unit is deleted. The draft indexed the object type table (an ARRAY at 0x69bfdc,
//   declared as a pointer variable) by the tag index, compared [0x719720] as a dword, left forward.k and
//   object_delete_unparented's EDI operand out.
// blam-cc: EAX -> placement_request, stack -> actor_variant_or_palette_tag, encounter_index,
//   squad_index, use_palette_entry, unit_type_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *object_data;     // 0x008603b0
extern Scenario *global_scenario;   // 0x00746f8c
extern int16_t network_game_mode; // 0x00719720 (compared as a word with 2)
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN
extern void objects_garbage_collection(void); // 0x4f9c60
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, EAX, stack
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index); // 0x426cf0, EAX, stack
extern datum_index actor_new_and_attach_to_unit(
    char reuse_existing, datum_index unit_index, datum_index actor_variant_tag,
    uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor,
    char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68); // 0x426ac0

#define TAG_DATA(h) ((uint8_t *)tag_instances[(h) & 0xffff].data)

datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index,
                                 int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index,
                                 const actor_placement_request *placement_request)
{
    const uint8_t *request = (const uint8_t *)placement_request;   // esi
    datum_index variant_tag = actor_variant_or_palette_tag;         // [ebp+0x8]
    uint8_t *variant;
    uint8_t *actor_definition;                                      // ebx
    object_placement_data placement;                                // [esp+0x18]
    float yaw;
    uint32_t role;
    datum_index unit_index;                                         // [esp+0x10]
    datum_index result;
    char swarm;                                                     // [esp+0x14]
    char start_active = 0;                                          // [esp+0xc]
    uint16_t initial_state = 0;                                     // ebx
    uint16_t return_state = 0;                                      // edi

    objects_garbage_collection();
    variant = TAG_DATA(variant_tag);
    if (use_palette_entry) {
        variant_tag = *(datum_index *)&((ActorVariant *)variant)->major_variant.tag_id;
        variant = TAG_DATA(variant_tag);
    }
    actor_definition = TAG_DATA(*(datum_index *)&((ActorVariant *)variant)->actor_definition.tag_id);
    object_placement_data_initialize(&placement, *(datum_index *)&((ActorVariant *)variant)->unit.tag_id, k_datum_index_none);
    yaw = ((struct actor_placement_request *)request)->yaw;
    placement.position = *(const real_point3d *)request;
    placement.permutation_group = (int16_t)unit_type_index;
    placement.forward.i = (float)cos((double)yaw);
    placement.forward.j = (float)sin((double)yaw);
    placement.forward.k = 0.0f;

    role = 3;
    if (network_game_mode == 2) {
        int16_t object_type = *(int16_t *)TAG_DATA(placement.definition_tag);

        if (*(int32_t *)((uint8_t *)object_type_definitions[object_type] + 0x10) != -1) {
            role = 0;
        }
    }
    unit_index = object_new_with_datum_role_control(&placement, role);
    if (unit_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    swarm = (char)((*(uint32_t *)actor_definition >> 0x1a) & 1);
    actor_apply_unit_definition_properties(variant_tag, unit_index);
    if (encounter_index != k_datum_index_none) {
        uint8_t *encounter = *(uint8_t **)((uint8_t *)global_scenario + 0x430) + (encounter_index & 0xffff) * 0xb0;
        uint8_t *squad = *(uint8_t **)(encounter + 0x84) + squad_index * 0xe8;

        initial_state = *(uint16_t *)(squad + 0x24);
        return_state = *(uint16_t *)(squad + 0x26);
        start_active = (char)((*(uint32_t *)(encounter + 0x20) >> 4) & 1);
    }
    if (((struct actor_placement_request *)request)->initial_state_override > 0) {
        initial_state = *(uint16_t *)&((struct actor_placement_request *)request)->initial_state_override;
    }
    if (*(const int16_t *)(request + 0x14) > 0) {
        return_state = *(const uint16_t *)(request + 0x14);
    }
    result = actor_new_and_attach_to_unit(swarm, unit_index, variant_tag, encounter_index, squad_index, 0,
        k_datum_index_none, start_active, initial_state, (int16_t)return_state, *(const uint16_t *)(request + 0x1a),
        (uint8_t)*(int8_t *)&((struct actor_placement_request *)request)->unknown_12);
    if (result == k_datum_index_none) {
        int32_t kind = *(int32_t *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data + 0x4);

        if (kind == 0) {
            object_delete_unparented(unit_index);
        } else if (kind != 3) {
            return result;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
