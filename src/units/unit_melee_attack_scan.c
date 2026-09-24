// unit_melee_attack_scan  (Ghidra: already named unit_melee_attack_scan)
// address 0x56f550, size 681 bytes
// name confidence: 0.5 (functions.md summary matches the code)
// rewrite confidence: 0.2 -- this is the least-understood file in the batch. The 5x5 grid scan,
//   its hit-test scratch record (local_50/local_18/local_1c/local_3c/local_54/uStack_4/uStack_c
//   in the original) and the decal helper breakable_surface_apply_damage could not be resolved to named fields in
//   the time available, and are reproduced with Ghidra's own local names rather than invented
//   ones. Only the object/unit fields the scan reads and the two damage_data records it builds
//   at the end are translated to named struct members.
// evidence: types/units.h unit_data.aiming_vector (0x23c), .current_weapon_index (0x2f2),
//   .weapons[4] (0x2f8), .controlling_player (0x218), .melee_state (0x289); types/objects.h
//   object.bounding_center (0x0a0), .location_leaf_index (0x098), .location_cluster_index
//   (0x09c), .name_index (0x0b8), .type (0x0b4), .velocity (0x068), .forward (0x074),
//   .parent_object (0x11c); types/tags.h Weapon.player_melee_response (0x394, tag_id at
//   absolute 0x3a0) and a second TagDependency immediately after it at 0x3b0 (this batch's
//   evidence calls it "actor_firing_parameters" per the earlier offset dump, but the value read
//   here, *(int*)(iVar15+0x3b0), is used as a decal-tag-shaped field -- kept as a raw offset,
//   UNSURE which of the two identifications is right); Unit.melee_damage (tag_id at absolute
//   0x294, the module-wide default).
// register convention: unit object index in EAX (param_1).
//   // blam-cc: EAX -> unit_index
// UNSURE: collision_test_movement_segment's scratch output record layout is reconstructed purely from Ghidra's own
//   stack-slot naming relative to the local_50 array this file passes as the buffer: a hit-kind
//   int16 at +0x00, a distance float at +0x14 (local_3c), a value at +0x34 (local_1c), a hit
//   object handle at +0x38 (local_18), a value at +0x44 (uStack_c) and a flags dword at +0x4c
//   (uStack_4). The large gaps between named slots are genuinely unread by this function, not
//   evidence the record is actually that sparse.
// UNSURE: breakable_surface_apply_damage (a "closest decal" trigger, called with two register-passed values) and
//   device_machine_melee_attacked (called with no visible arguments at all) are out of this module's range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *globals_tag_data;    // 0x00746fa0
extern char s_primary_eye_marker[];            // 0x0066bfa0, a marker-name string ("melee"-adjacent)

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t param_3,
                                 int16_t param_4, int16_t param_5, uint32_t param_6); // 0x4ee5e0
extern void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id); // 0x56f210
extern void breakable_surface_apply_damage(int16_t a, int32_t b); // 0x4ffde0, UNSURE signature
extern void device_machine_melee_attacked(void); // 0x44b5d0, UNSURE signature
extern void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse); // 0x571cb0, this batch

// Sweeps a 5x5 grid of hit tests in a cone around the unit's aim direction to find the nearest
// melee target (an object) or surface (for a decal), then applies damage to the best object
// candidate found or triggers a decal at the best surface point, and finally applies a fixed
// area-effect "push" damage at the aim origin when a weapon-defined secondary melee effect
// exists.
void unit_melee_attack_scan(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    object_marker melee_marker;
    real_vector3d perp;
    real_point3d origin;
    real_vector3d aim = *(real_vector3d *)((uint8_t *)obj + 0x23c);

    uint32_t best_object = 0xffffffff;
    int32_t best_object_distance = -1;      // local_124
    int32_t best_decal = -1;                // local_e8
    int32_t best_decal_extra = 0;           // uStack_d4
    int32_t secondary_damage_effect = -1;   // local_f4
    int16_t best_object_type = 0;           // local_ec
    float best_object_fraction = 0.0f;      // local_54

    object_get_node_local_transform(unit_index, s_primary_eye_marker, &melee_marker, 1);
    origin = melee_marker.node_transform.position;

    vector3d_build_perpendicular(&perp, &aim);
    vector3d_normalize_with_length(&perp);

    for (int32_t row = -2; row <= 2; row++) {
        for (int32_t col = -2; col <= 2; col++) {
            real_vector3d delta;
            uint8_t scratch[0x50]; // UNSURE, see header: hit_kind @0, distance @0x14,
                                    // local_1c @0x34, hit_object @0x38, uStack_c @0x44,
                                    // flags @0x4c (offsets per Ghidra's own local layout)
            int16_t *hit_kind = (int16_t *)scratch;

            // UNSURE: reproduced literally from the Ghidra arithmetic (fVar2..fVar7 are aim.j,
            // aim.k, aim.k, aim.i, aim.i, aim.j respectively, and local_104/108/10c are
            // perp.i/j/k). The index pattern below is not a textbook cross product; it is kept
            // exactly as decompiled rather than "corrected" into one.
            delta.i = aim.i * 0.8f + ((float)col * (perp.i * aim.j - perp.j * aim.k) + (float)row * perp.k) * 0.1f;
            delta.j = aim.j * 0.8f + ((float)col * (perp.k * aim.k - perp.i * aim.i) + (float)row * perp.j) * 0.1f;
            delta.k = aim.k * 0.8f + ((float)col * (perp.j * aim.i - perp.k * aim.j) + (float)row * perp.i) * 0.1f;

            if (collision_test_movement_segment(0x1000e9, &origin, &delta, unit_index, scratch) != 0) {
                if (*hit_kind == 2) {
                    if (best_object == 0xffffffff) {
                        int32_t local_1c = *(int32_t *)(scratch + 0x34);
                        uint32_t flags = *(uint32_t *)(scratch + 0x4c);   // uStack_4
                        best_object_distance = local_1c;
                        if ((flags & 8) != 0) {
                            best_decal = (best_decal & 0xffff0000) |
                                         (uint32_t)*(uint8_t *)(scratch + 0x4d); // byte 1 of uStack_4
                            best_decal_extra = *(int32_t *)(scratch + 0x44);     // uStack_c
                        }
                    }
                } else if (*hit_kind == 3) {
                    datum_index candidate = *(datum_index *)(scratch + 0x38); // local_18
                    object *cand_obj = ((object_header *)object_data->data)[candidate & 0xffff].data;
                    uint32_t candidate_index = candidate;

                    if (cand_obj->type != 2 && cand_obj->parent_object != k_datum_index_none) {
                        candidate_index = cand_obj->parent_object;
                        cand_obj = ((object_header *)object_data->data)[candidate_index & 0xffff].data;
                    }

                    if (best_object == 0xffffffff ||
                        (cand_obj->type == 0 &&
                         ((best_object_type == 0 && *(float *)(scratch + 0x14) < best_object_fraction) ||
                          best_object_type != 0))) {
                        best_object_type = cand_obj->type;
                        best_object_distance = *(int32_t *)(scratch + 0x34); // local_1c
                        best_object_fraction = *(float *)(scratch + 0x14);   // local_3c
                        best_object = candidate_index;
                    }
                }
            }
        }
    }

    {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        int32_t weapon_response_tag = -1;

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
                Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                weapon_response_tag = *(int32_t *)&weapon_tag->player_melee_response.tag_id;
                secondary_damage_effect = *(int32_t *)((uint8_t *)weapon_tag + 0x3b0); // UNSURE field
            }
        }
        int32_t damage_effect_tag = (weapon_response_tag != -1) ? weapon_response_tag
                                                                  : *(int32_t *)&tag->melee_damage.tag_id;

        if (best_object != 0xffffffff) {
            object *best_obj = ((object_header *)object_data->data)[best_object & 0xffff].data;
            if (best_obj->type == 1 && best_obj->network_role != 1) {
                // UNSURE: both arguments are register-only at this call site; the struck
                // vehicle is the more plausible impulse target than the attacker, and the aim
                // direction the most plausible impulse vector, but neither is confirmed.
                unit_apply_impulse_to_seat(best_object, &aim);
            }
        }

        if (damage_effect_tag != -1) {
            damage_data dd = {0};

            dd.damage_effect_tag = damage_effect_tag;
            dd.flags = 1;
            dd.responsible_player = obj->name_index; // UNSURE: matches unit_cause_melee_damage's
                                                      //   odd team_index/name_index pairing
            dd.responsible_object = unit_index;
            dd.team_index = (int16_t)obj->name_index;
            dd.location_leaf_index = obj->location_leaf_index;
            *(int32_t *)&dd.location_cluster_index = *(int32_t *)&obj->location_cluster_index;
            dd.epicentre = origin;                 // the melee marker's world position
            dd.origin = obj->bounding_center;
            dd.direction = aim;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            dd.unknown_4c = (int16_t)best_object_distance;

            if (best_object == 0xffffffff) {
                if ((int16_t)best_decal != -1) {
                    breakable_surface_apply_damage((int16_t)best_decal, best_decal_extra);
                }
            } else {
                object *best_obj = ((object_header *)object_data->data)[best_object & 0xffff].data;
                if (best_obj->type == 7) {
                    device_machine_melee_attacked();
                }
                if (*(float *)(globals_tag_data + 0x174 + 0x34) > 0.0f) {
                    float f = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                               obj->forward.k * obj->velocity.k) * 30.0f /
                              *(float *)(globals_tag_data + 0x174 + 0x34);
                    dd.random_blend = (f < 0.0f) ? 0.0f : (f > 1.0f ? 1.0f : f);
                }
                if (obj->type == 0 && *(int8_t *)((uint8_t *)obj + 0x501) > 0x0f) {
                    // UNSURE: object+0x501 has no name in types/objects.h; per the original this
                    // is only checked when this unit is a biped.
                    dd.random_blend = 1.5f;
                }
                if (best_obj->type == 0) {
                    object_apply_damage(&dd, best_object, -1, -1, -1, 0);
                }
            }
        }
        if ((int16_t)best_object_distance != -1) {
            unit_trigger_material_hit_effect((int16_t)best_object_distance, k_datum_index_none);
            if (secondary_damage_effect != -1) {
                damage_data dd2 = {0};
                dd2.damage_effect_tag = secondary_damage_effect;
                dd2.team_index = -1;
                dd2.responsible_player = k_datum_index_none;
                dd2.responsible_object = k_datum_index_none;
                dd2.location_leaf_index = 0;
                *(int16_t *)&dd2.location_cluster_index = -1;
                dd2.epicentre = obj->bounding_center;
                dd2.origin = obj->bounding_center;
                dd2.direction.i = -aim.i;
                dd2.direction.j = -aim.j;
                dd2.direction.k = -aim.k;
                dd2.random_blend = 1.0f;
                dd2.multiplier = 1.0f;
                dd2.unknown_4c = -1;
                object_apply_damage(&dd2, unit_index, -1, -1, -1, 0);
            }
        }
        unit->melee_state = 0;
    }
}

#if 0
Original Ghidra decompilation (0x56f550):

void unit_melee_attack_scan(uint param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  short sVar8;
  uint *puVar9;
  uint uVar10;
  float fVar11;
  float fVar12;
  char cVar13;
  int iVar14;
  int iVar15;
  uint uVar16;
  int *piVar17;
  int local_12c;
  uint local_128;
  undefined4 local_124;
  int local_120;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f8;
  int local_f4;
  int local_f0;
  short local_ec;
  undefined4 local_e8;
  int local_e4;
  undefined4 local_e0;
  undefined4 local_dc;
  undefined4 local_d8;
  undefined4 uStack_d4;
  int local_d0;
  int local_cc;
  int local_c8 [4];
  undefined2 uStack_b8;
  uint uStack_b4;
  uint uStack_b0;
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_a0;
  uint uStack_9c;
  uint uStack_98;
  float fStack_94;
  float fStack_90;
  float fStack_8c;
  float fStack_88;
  undefined4 uStack_84;
  short sStack_7c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  float local_54;
  short local_50 [10];
  float local_3c;
  undefined4 local_1c;
  uint local_18;
  undefined4 uStack_c;
  uint uStack_4;

  iVar15 = (param_1 & 0xffff) * 0xc;
  puVar9 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar15);
  local_cc = *(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_128 = 0xffffffff;
  local_124 = 0xffffffff;
  local_e8 = 0xffffffff;
  local_f4 = -1;
  local_d0 = iVar15;
  object_get_node_local_transform(param_1,&DAT_0066bfa0,local_c8,1);
  local_dc = local_64;
  local_d8 = local_60;
  pfVar1 = (float *)(puVar9 + 0x8f);
  local_e0 = local_68;
  vector3d_build_perpendicular();
  vector3d_normalize_with_length();
  local_12c = -2;
  fVar2 = (float)puVar9[0x90];
  local_e4 = 5;
  fVar3 = (float)puVar9[0x91];
  fVar4 = (float)puVar9[0x91];
  fVar5 = *pfVar1;
  fVar6 = *pfVar1;
  fVar7 = (float)puVar9[0x90];
  do {
    fVar11 = (float)local_12c;
    local_120 = -2;
    local_f0 = 5;
    do {
      fVar12 = (float)local_120;
      local_100 = *pfVar1 * 0.8 +
                  (fVar12 * (local_104 * fVar2 - local_108 * fVar3) + fVar11 * local_10c) * 0.1;
      local_fc = (float)puVar9[0x90] * 0.8 +
                 (fVar12 * (local_10c * fVar4 - local_104 * fVar5) + fVar11 * local_108) * 0.1;
      local_f8 = (float)puVar9[0x91] * 0.8 +
                 (fVar12 * (local_108 * fVar6 - local_10c * fVar7) + fVar11 * local_104) * 0.1;
      cVar13 = FUN_00505880(0x1000e9,&local_e0,&local_100,param_1,local_50);
      if (cVar13 != '\0') {
        if (local_50[0] == 2) {
          if ((local_128 == 0xffffffff) && (local_124 = local_1c, (uStack_4 & 8) != 0)) {
            local_e8 = CONCAT22(local_e8._2_2_,(ushort)(byte)(uStack_4 >> 8));
            uStack_d4 = uStack_c;
          }
        }
        else if (local_50[0] == 3) {
          iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_18 & 0xffff) * 0xc);
          uVar16 = local_18;
          if ((*(short *)(iVar14 + 0xb4) != 2) &&
             (uVar10 = *(uint *)(iVar14 + 0x11c), uVar10 != 0xffffffff)) {
            iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar10 & 0xffff) * 0xc);
            uVar16 = uVar10;
          }
          if ((local_128 == 0xffffffff) ||
             ((*(short *)(iVar14 + 0xb4) == 0 &&
              (((local_ec == 0 && (local_3c < local_54)) || (local_ec != 0)))))) {
            local_ec = *(short *)(iVar14 + 0xb4);
            local_124 = local_1c;
            local_54 = local_3c;
            local_128 = uVar16;
          }
        }
      }
      local_120 = local_120 + 1;
      local_f0 = local_f0 + -1;
    } while (local_f0 != 0);
    local_12c = local_12c + 1;
    local_e4 = local_e4 + -1;
  } while (local_e4 != 0);
  iVar15 = *(int *)(iVar15 + 8 + *(int *)(DAT_008603b0 + 0x34));
  sVar8 = *(short *)(iVar15 + 0x2f2);
  if ((sVar8 != -1) && (uVar16 = *(uint *)(iVar15 + 0x2f8 + sVar8 * 4), uVar16 != 0xffffffff)) {
    iVar15 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar16 & 0xffff) * 0xc) &
                      0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar14 = *(int *)(iVar15 + 0x3a0);
    local_f4 = *(int *)(iVar15 + 0x3b0);
    if (iVar14 != -1) goto LAB_0056f8cf;
  }
  iVar14 = *(int *)(local_cc + 0x294);
LAB_0056f8cf:
  if (((local_128 != 0xffffffff) &&
      (iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_128 & 0xffff) * 0xc),
      *(short *)(iVar15 + 0xb4) == 1)) && (*(int *)(iVar15 + 4) != 1)) {
    FUN_00571cb0();
  }
  if (iVar14 != -1) {
    piVar17 = local_c8;
    for (iVar15 = 0x15; iVar15 != 0; iVar15 = iVar15 + -1) {
      *piVar17 = 0;
      piVar17 = piVar17 + 1;
    }
    uStack_b4 = puVar9[0x26];
    local_c8[1] = local_c8[1] | 1;
    uStack_b0 = puVar9[0x27];
    uStack_b8 = (undefined2)puVar9[0x2e];
    local_c8[3] = param_1;
    local_c8[2] = puVar9[0x86];
    uStack_ac = local_e0;
    uStack_a8 = local_dc;
    uStack_a4 = local_d8;
    uStack_a0 = puVar9[0x28];
    uStack_9c = puVar9[0x29];
    uStack_98 = puVar9[0x2a];
    fStack_94 = *pfVar1;
    fStack_90 = (float)puVar9[0x90];
    fStack_8c = (float)puVar9[0x91];
    fStack_88 = 1.0;
    uStack_84 = 0x3f800000;
    sStack_7c = (short)local_124;
    local_c8[0] = iVar14;
    if (local_128 == 0xffffffff) {
      if ((short)local_e8 != -1) {
        FUN_004ffde0(local_e8,uStack_d4);
      }
    }
    else {
      iVar15 = (local_128 & 0xffff) * 0xc;
      if (*(short *)(*(int *)(iVar15 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0xb4) == 7) {
        FUN_0044b5d0();
      }
      if (0.0 < *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x34)) {
        fStack_88 = (((float)puVar9[0x1d] * (float)puVar9[0x1a] +
                     (float)puVar9[0x1b] * (float)puVar9[0x1e] +
                     (float)puVar9[0x1c] * (float)puVar9[0x1f]) * 30.0) /
                    *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x34);
        if (0.0 <= fStack_88) {
          if (1.0 < fStack_88) {
            fStack_88 = 1.0;
          }
        }
        else {
          fStack_88 = 0.0;
        }
      }
      if (((short)puVar9[0x2d] == 0) &&
         ('\x0f' < *(char *)(*(int *)(local_d0 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x501))) {
        fStack_88 = 1.5;
      }
      if (*(short *)(*(int *)(iVar15 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0xb4) == 0) {
        object_apply_damage(local_c8,local_128,0xffffffff,0xffffffff,0xffffffff,0);
      }
    }
  }
  if (((short)local_124 != -1) && (FUN_0056f210(), local_f4 != -1)) {
    fVar2 = *pfVar1;
    piVar17 = local_c8;
    for (iVar15 = 0x15; iVar15 != 0; iVar15 = iVar15 + -1) {
      *piVar17 = 0;
      piVar17 = piVar17 + 1;
    }
    fStack_90 = -(float)puVar9[0x90];
    local_c8[1] = local_c8[1] | 8;
    fStack_8c = -(float)puVar9[0x91];
    uStack_a0 = puVar9[0x28];
    uStack_9c = puVar9[0x29];
    uStack_98 = puVar9[0x2a];
    uStack_ac = puVar9[0x28];
    uStack_a8 = puVar9[0x29];
    uStack_a4 = puVar9[0x2a];
    sStack_7c = 0xffff;
    local_c8[2] = 0xffffffff;
    local_c8[3] = 0xffffffff;
    uStack_b8 = 0xffff;
    uStack_b0 = CONCAT22(uStack_b0._2_2_,0xffff);
    local_c8[0] = local_f4;
    fStack_88 = 1.0;
    uStack_84 = 0x3f800000;
    fStack_94 = -fVar2;
    object_apply_damage(local_c8,param_1,0xffffffff,0xffffffff,0xffffffff,0);
  }
  *(undefined1 *)((int)puVar9 + 0x289) = 0;
  return;
}
#endif
