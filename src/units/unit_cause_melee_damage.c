// unit_cause_melee_damage  (Ghidra: already named unit_cause_melee_damage)
// address 0x56f2d0, size 626 bytes
// name confidence: 0.85 (cea-pdb hint via the "melee" string; functions.md summary matches)
// rewrite confidence: 0.45
// evidence: types/tags.h Unit.melee_damage (0x288, TagDependency, tag_id at +0xc = absolute
//   0x294), Weapon.weapon_flags (0x308, bit 0x8000 = ais_use_weapon_melee_damage),
//   Weapon.player_melee_response (0x394, tag_id at absolute 0x3a0); types/objects.h
//   object.bounding_center (0x0a0), .location_leaf_index (0x098), .location_cluster_index
//   (0x09c, copied as a full dword together with the following padding), .name_index (0x0b8);
//   types/objects.h damage_data (every field below matches its offset, as in
//   src/units/unit_apply_fall_damage.c); types/units.h unit_data.controlling_player (0x218),
//   .current_weapon_index (0x2f2), .weapons[4] (0x2f8), .melee_state (0x289); callees
//   object_get_node_local_transform (0x4f6080), object_apply_damage, damage_apply_area_effect,
//   unit_trigger_material_hit_effect (0x56f210, this batch).
// register convention: unit object index in EAX (param_1), a "quiet" byte flag in a second
//   register (param_2), then four object_apply_damage passthrough parameters and the target
//   object index as stack parameters (param_3..param_7).
//   // blam-cc: EAX -> unit_index, second register -> suppress_effect,
//   //           stack -> (target_object_index, damage_node, damage_param5, damage_param6, damage_param7)
// UNSURE: damage_data.team_index (0x10) is filled from object.owner_team (0x0b8), which reads
//   oddly for a "team" field but is reproduced literally; this may be an artifact of the
//   compiler reusing damage_data.team_index's storage for something else, or the field
//   identification for one of the two is wrong.
// UNSURE: pfVar1's role as "attacker origin" and the marker's node_transform.position as "melee
//   marker world position" follow the same stack-overlay reasoning used in
//   unit_update_look_delta_controls.c; the object_marker local buffer here is 96 bytes, exactly
//   covering through node_transform's position field.
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                 int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void damage_apply_area_effect(damage_data *request, uint32_t param_2); // 0x4edd30  // real signature (damage_apply_area_effect.c): void damage_apply_area_effect(damage_data *dd); Ghidra recovered 2 of 1 args at this call site
extern void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index); // 0x56f210

// Performs the unit's melee attack: locates the "melee" marker (falling back to the unit's
// bounding center, and re-checking line of sight from the center to the marker), resolves the
// melee damage effect (the Unit tag's default, or the current weapon's own response effect when
// its ais_use_weapon_melee_damage flag is set), and applies it either to a specific target
// object or as an area effect at the impact point. Clears melee_state either way.
void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect, uint32_t target_object_index,
                              int16_t damage_param4, int16_t damage_param5, int16_t damage_param6,
                              uint32_t damage_param7)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    real_point3d origin_pos = obj->bounding_center;
    real_point3d target_pos;
    object_marker melee_marker;
    int16_t found;
    datum_index damage_effect;
    damage_data dd;

    if (*(int32_t *)&tag->melee_damage.tag_id == -1) {
        unit_data *unit0 = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit0->melee_state = 0;
        return;
    }

    found = object_get_node_local_transform(unit_index, "melee", &melee_marker, 1);
    if (found == 1) {
        real_vector3d delta;
        uint8_t scratch[0x54]; // UNSURE: raw scratch buffer for collision_test_movement_segment

        target_pos = melee_marker.node_transform.position;
        delta.i = target_pos.x - origin_pos.x;
        delta.j = target_pos.y - origin_pos.y;
        delta.k = target_pos.z - origin_pos.z;

        if (collision_test_movement_segment(0x1000e9, &origin_pos, &delta, 0xffffffff, scratch) != 0) {
            target_pos = origin_pos; // blocked: fall back to the unit's own center
        }
    } else {
        target_pos = origin_pos;
    }

    obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        damage_effect = *(datum_index *)&tag->melee_damage.tag_id;

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
                Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((weapon_tag->weapon_flags & 0x8000) != 0) {
                    damage_effect = *(datum_index *)&weapon_tag->player_melee_response.tag_id;
                }
            }
        }

        dd.damage_effect_tag = damage_effect;
        dd.responsible_player = unit->controlling_player;
        dd.responsible_object = unit_index;
        dd.team_index = obj->owner_team; // UNSURE, see header
        dd.location_leaf_index = obj->location_leaf_index;
        *(int32_t *)&dd.location_cluster_index = *(int32_t *)&obj->location_cluster_index;
        dd.epicentre = target_pos;
        dd.origin = origin_pos;
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.material_type = -1;

        if (target_object_index == 0xffffffff) {
            damage_apply_area_effect(&dd, 0xffffffff);
        } else {
            object_apply_damage(&dd, target_object_index, damage_param4, damage_param5, damage_param6, damage_param7);
        }

        if (suppress_effect == 0 && dd.material_type != -1) {
            unit_trigger_material_hit_effect(dd.material_type, damage_effect, unit_index); // 0x56f513: ECX the damage effect, EDX the unit
        }

        unit->melee_state = 0;
    }
}

#if 0
Original Ghidra decompilation (0x56f2d0):

void unit_cause_melee_damage
               (uint param_1,char param_2,int param_3,undefined4 param_4,undefined4 param_5,
               undefined4 param_6,undefined4 param_7)

{
  float *pfVar1;
  uint *puVar2;
  uint uVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint *puVar9;
  float local_d8;
  float local_d4;
  float local_d0;
  float local_cc;
  float local_c8;
  float local_c4;
  uint local_c0 [4];
  undefined2 local_b0;
  uint local_ac;
  uint local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  uint local_94;
  uint local_90;
  undefined4 local_80;
  undefined4 local_7c;
  short local_74;
  undefined1 local_6c [96];
  float local_c;
  float local_8;
  float local_4;

  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  iVar6 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar6 + 0x294) != -1) {
    sVar5 = object_get_node_local_transform(param_1,"melee",local_6c,1);
    pfVar1 = (float *)(puVar2 + 0x28);
    if (sVar5 == 1) {
      local_d8 = local_c;
      local_cc = local_c - *pfVar1;
      local_d4 = local_8;
      local_d0 = local_4;
      local_c8 = local_8 - (float)puVar2[0x29];
      local_c4 = local_4 - (float)puVar2[0x2a];
      cVar4 = FUN_00505880(0x1000e9,pfVar1,&local_cc,0xffffffff,local_c0);
      if (cVar4 != '\0') {
        local_d8 = *pfVar1;
        local_d4 = (float)puVar2[0x29];
        local_d0 = (float)puVar2[0x2a];
      }
    }
    else {
      local_d8 = *pfVar1;
      local_d4 = (float)puVar2[0x29];
      local_d0 = (float)puVar2[0x2a];
    }
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
    sVar5 = *(short *)(iVar8 + 0x2f2);
    uVar7 = *(uint *)(iVar6 + 0x294);
    if (((sVar5 != -1) && (uVar3 = *(uint *)(iVar8 + 0x2f8 + sVar5 * 4), uVar3 != 0xffffffff)) &&
       (iVar6 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc) &
                         0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       (char)((uint)*(undefined4 *)(iVar6 + 0x308) >> 8) < '\0')) {
      uVar7 = *(uint *)(iVar6 + 0x3a0);
    }
    uVar3 = puVar2[0x27];
    puVar9 = local_c0;
    for (iVar6 = 0x15; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar9 = 0;
      puVar9 = puVar9 + 1;
    }
    local_b0 = (undefined2)puVar2[0x2e];
    local_ac = puVar2[0x26];
    local_c0[2] = puVar2[0x86];
    local_a0 = local_d4;
    local_94 = puVar2[0x29];
    local_a4 = local_d8;
    local_98 = *pfVar1;
    local_9c = local_d0;
    local_90 = puVar2[0x2a];
    local_74 = -1;
    local_80 = 0x3f800000;
    local_7c = 0x3f800000;
    local_c0[3] = param_1;
    local_c0[0] = uVar7;
    local_a8 = uVar3;
    if (param_3 == -1) {
      damage_apply_area_effect(local_c0,0xffffffff);
    }
    else {
      object_apply_damage(local_c0,param_3,param_4,param_5,param_6,param_7);
    }
    if ((param_2 == '\0') && (local_74 != -1)) {
      FUN_0056f210();
    }
    *(undefined1 *)((int)puVar2 + 0x289) = 0;
    return;
  }
  *(undefined1 *)((int)puVar2 + 0x289) = 0;
  return;
}
#endif
