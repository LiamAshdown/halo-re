// unit_can_see_point  (Ghidra: already named unit_can_see_point)
// address 0x56f800, size 1151 bytes
// name confidence: 0.1   rewrite confidence: 1.0 (FRAGMENT: 0x56f800 is inside unit_melee_attack_scan 0x56f550..0x56fc7e, whose C covers this code; only jumps from inside that function land here)
// name confidence: 0.55 (functions.md summary: structurally plausible visibility/line-of-sight
//   raycast helper)
// rewrite confidence: 0.1 -- shares its entire tail (the weapon-response damage-effect lookup,
//   the FUN_00571cb0 impulse call, the object_apply_damage build, the decal/device_machine_melee_attacked
//   dispatch, and the final self-push damage block) almost verbatim with
//   unit_melee_attack_scan.c (0x56f550, this batch), but its *entry* is far more badly split:
//   the grid-scan basis vectors (the "perp"/"up"-shaped inputs the melee scan builds itself via
//   vector3d_build_perpendicular) arrive here as bare in_stack_ values with no visible
//   construction at all, and the outer loop is a "do { ... } while(true)" that only terminates
//   through returns buried three loops deep -- the unmistakable signature of Ghidra having cut
//   this function's real entry point off mid-prologue. Modeled here with the missing basis
//   vectors as explicit parameters rather than guessed constants.
// evidence: identical field mapping to unit_melee_attack_scan.c for every offset shared between
//   the two decompiles (unit_data.current_weapon_index 0x2f2, .weapons[4] 0x2f8,
//   .controlling_player 0x218, .melee_state 0x289; object.type 0x0b4, .parent_object 0x11c,
//   .name_index 0x0b8, .location_leaf_index 0x098, .location_cluster_index 0x09c,
//   .velocity 0x068, .forward 0x074; Weapon.player_melee_response tag_id at 0x3a0 and the field
//   at 0x3b0; Unit.melee_damage tag_id at 0x294).
// register convention: UNRESOLVED for the scan setup (unaff_EBX/EBP/ESI); the unit object index
//   survives as in_stack_00000144 throughout and is modeled here as the first parameter.
//   // blam-cc: UNSURE -- see header
// UNSURE: essentially the entire grid-scan section (the row/col bookkeeping, the perp/up basis,
//   and collision_test_movement_segment's scratch layout) is reproduced as literally as Ghidra's own in_stack_
//   names allow rather than restructured, since the true entry conditions are not recoverable.
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *global_globals;    // 0x00746fa0

extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                 int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse); // 0x571cb0, this batch
extern void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index); // 0x56f210, this batch
extern void breakable_surface_apply_damage(damage_data *damage, int32_t surface_index,
    int32_t collision_surface_index); // 0x4ffde0, EBX, stack
extern void device_machine_melee_attacked(uint32_t object_index); // 0x44b5d0, ECX

// Tests line-of-sight/visibility from the unit toward a target point along a caller-supplied
// grid basis, applying melee-response damage and decal/light effects along the trace.
// UNSURE: see file header -- the scan's own basis vectors could not be recovered and are
// parameters here rather than constants this function derives itself.
void unit_can_see_point(uint32_t unit_index, real_vector3d *target_direction,
                         real_vector3d *perp, real_vector3d *up)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    uint32_t best_object = 0xffffffff;
    int32_t best_object_distance = -1;
    int32_t best_decal = -1;
    int32_t best_decal_extra = 0;
    int16_t best_object_type = 0;
    float best_object_fraction = 0.0f;

    for (int32_t row = -2; row <= 2; row++) {
        for (int32_t col = -2; col <= 2; col++) {
            real_vector3d delta;
            uint8_t scratch[0x50]; // UNSURE, see unit_melee_attack_scan.c's identical layout note
            int16_t *hit_kind = (int16_t *)scratch;

            delta.i = target_direction->i * 0.8f + ((float)col * perp->i + (float)row * up->i) * 0.1f; // UNSURE
            delta.j = target_direction->j * 0.8f + ((float)col * perp->j + (float)row * up->j) * 0.1f;
            delta.k = target_direction->k * 0.8f + ((float)col * perp->k + (float)row * up->k) * 0.1f;

            if (collision_test_movement_segment(0x1000e9, (real_point3d *)target_direction, &delta, unit_index, scratch) == 0) {
                continue;
            }

            if (*hit_kind == 3) {
                datum_index candidate = *(datum_index *)(scratch + 0x38);
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
                    best_object_distance = *(int32_t *)(scratch + 0x34);
                    best_object_fraction = *(float *)(scratch + 0x14);
                    best_object = candidate_index;
                }
            }
        }
    }

    {
        int32_t weapon_response_tag = -1;
        int32_t secondary_damage_effect = -1;

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
                Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                weapon_response_tag = *(int32_t *)&weapon_tag->player_melee_response.tag_id;
                secondary_damage_effect = *(int32_t *)&((struct Weapon *)weapon_tag)->player_melee_response.tag_id; // UNSURE field
            }
        }
        int32_t damage_effect_tag = (weapon_response_tag != -1) ? weapon_response_tag
                                                                  : *(int32_t *)&tag->melee_damage.tag_id;

        if (best_object != 0xffffffff) {
            object *best_obj = ((object_header *)object_data->data)[best_object & 0xffff].data;
            if (best_obj->type == 1 && best_obj->network_role != 1) {
                unit_apply_impulse_to_seat(best_object, target_direction); // UNSURE impulse vector
            }
        }

        if (damage_effect_tag != -1) {
            damage_data dd = {0};

            dd.damage_effect_tag = damage_effect_tag;
            dd.flags = 1;
            dd.responsible_player = obj->owner_team; // UNSURE, matches unit_cause_melee_damage.c
            dd.responsible_object = unit_index;
            dd.team_index = (int16_t)obj->owner_team;
            dd.location_leaf_index = obj->location_leaf_index;
            *(int32_t *)&dd.location_cluster_index = *(int32_t *)&obj->location_cluster_index;
            dd.epicentre = *(real_point3d *)target_direction;
            dd.origin = obj->bounding_center;
            dd.direction = *target_direction;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            dd.material_type = (int16_t)best_object_distance;

            if (best_object == 0xffffffff) {
                if ((int16_t)best_decal != -1) {
                    breakable_surface_apply_damage(&dd, best_decal, best_decal_extra); // FIXED: EBX = &dd (0x56fa47)
                }
            } else {
                object *best_obj = ((object_header *)object_data->data)[best_object & 0xffff].data;
                if (best_obj->type == 7) {
                    device_machine_melee_attacked(best_object); // FIXED: ECX = the hit object (0x56fa82)
                }
                if (*(float *)(global_globals + 0x174 + 0x34) > 0.0f) {
                    float f = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                               obj->forward.k * obj->velocity.k) * 30.0f /
                              *(float *)(global_globals + 0x174 + 0x34);
                    dd.random_blend = (f < 0.0f) ? 0.0f : (f > 1.0f ? 1.0f : f);
                }
                if (obj->type == 0 && *(int8_t *)((uint8_t *)obj + 0x501) > 0x0f) {
                    dd.random_blend = 1.5f;
                }
                if (best_obj->type == 0) {
                    object_apply_damage(&dd, best_object, -1, -1, -1, 0);
                }
            }
        }

        if ((int16_t)best_object_distance != -1) {
            unit_trigger_material_hit_effect((int16_t)best_object_distance, k_datum_index_none, k_datum_index_none); // STUCK: 0x56fb73 passes ECX ebx / EDX [esp+0x144], not yet traced (see build/overnight_log.md)
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
                dd2.direction.i = -target_direction->i;
                dd2.direction.j = -target_direction->j;
                dd2.direction.k = -target_direction->k;
                dd2.random_blend = 1.0f;
                dd2.multiplier = 1.0f;
                dd2.material_type = -1;
                object_apply_damage(&dd2, unit_index, -1, -1, -1, 0);
            }
        }
        unit->melee_state = 0;
    }
}

#if 0
Original Ghidra decompilation (0x56f800):

void unit_can_see_point(void)

{
  float fVar1;
  short sVar2;
  uint uVar3;
  char cVar4;
  int iVar5;
  int unaff_EBX;
  int iVar6;
  int unaff_EBP;
  float *unaff_ESI;
  uint uVar7;
  undefined4 *puVar8;
  int in_stack_00000014;
  uint in_stack_00000018;
  undefined2 uStack0000001c;
  int in_stack_00000020;
  float in_stack_00000024;
  float in_stack_00000028;
  float in_stack_0000002c;
  float in_stack_00000030;
  float in_stack_00000034;
  float in_stack_00000038;
  float in_stack_0000003c;
  float in_stack_00000040;
  float in_stack_00000044;
  float in_stack_00000048;
  int in_stack_0000004c;
  int in_stack_00000050;
  short in_stack_00000054;
  undefined4 in_stack_00000058;
  int in_stack_0000005c;
  undefined4 in_stack_00000060;
  undefined4 in_stack_00000064;
  undefined4 in_stack_00000068;
  undefined4 in_stack_0000006c;
  int in_stack_00000070;
  int in_stack_00000074;
  int in_stack_00000078;
  uint in_stack_0000007c;
  undefined4 in_stack_00000080;
  undefined4 in_stack_00000084;
  undefined2 in_stack_00000088;
  undefined4 in_stack_0000008c;
  undefined4 in_stack_00000090;
  undefined4 in_stack_00000094;
  undefined4 in_stack_00000098;
  undefined4 in_stack_0000009c;
  undefined4 in_stack_000000a0;
  undefined4 in_stack_000000a4;
  undefined4 in_stack_000000a8;
  float in_stack_000000ac;
  float in_stack_000000b0;
  float in_stack_000000b4;
  float in_stack_000000b8;
  undefined4 in_stack_000000bc;
  undefined2 in_stack_000000c4;
  float in_stack_000000ec;
  short in_stack_000000f0;
  float in_stack_00000104;
  undefined4 in_stack_00000124;
  uint in_stack_00000128;
  undefined4 in_stack_00000134;
  uint in_stack_0000013c;
  undefined4 in_stack_00000144;

  do {
    _uStack0000001c = in_stack_00000124;
    if ((in_stack_0000013c & 8) != 0) {
      in_stack_00000058 = CONCAT22(in_stack_00000058._2_2_,(ushort)(byte)(in_stack_0000013c >> 8));
      in_stack_0000006c = in_stack_00000134;
    }
LAB_0056f82a:
    do {
      do {
        in_stack_00000020 = in_stack_00000020 + 1;
        in_stack_00000050 = in_stack_00000050 + -1;
        if (in_stack_00000050 == 0) {
          in_stack_00000014 = in_stack_00000014 + 1;
          in_stack_0000005c = in_stack_0000005c + -1;
          if (in_stack_0000005c == 0) {
            iVar5 = *(int *)(unaff_EBX + 8 + *(int *)(DAT_008603b0 + 0x34));
            sVar2 = *(short *)(iVar5 + 0x2f2);
            if ((sVar2 != -1) && (uVar7 = *(uint *)(iVar5 + 0x2f8 + sVar2 * 4), uVar7 != 0xffffffff)
               ) {
              iVar5 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                           (uVar7 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                              DAT_0087bc14);
              iVar6 = *(int *)(iVar5 + 0x3a0);
              in_stack_0000004c = *(int *)(iVar5 + 0x3b0);
              if (iVar6 != -1) goto LAB_0056f8cf;
            }
            iVar6 = *(int *)(in_stack_00000074 + 0x294);
LAB_0056f8cf:
            if (((in_stack_00000018 != 0xffffffff) &&
                (iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (in_stack_00000018 & 0xffff) * 0xc), *(short *)(iVar5 + 0xb4) == 1)
                ) && (*(int *)(iVar5 + 4) != 1)) {
              FUN_00571cb0();
            }
            if (iVar6 != -1) {
              puVar8 = &stack0x00000078;
              for (iVar5 = 0x15; iVar5 != 0; iVar5 = iVar5 + -1) {
                *puVar8 = 0;
                puVar8 = puVar8 + 1;
              }
              in_stack_0000008c = *(undefined4 *)(unaff_EBP + 0x98);
              in_stack_0000007c = in_stack_0000007c | 1;
              in_stack_00000090 = *(undefined4 *)(unaff_EBP + 0x9c);
              in_stack_00000088 = *(undefined2 *)(unaff_EBP + 0xb8);
              in_stack_00000084 = in_stack_00000144;
              in_stack_00000080 = *(undefined4 *)(unaff_EBP + 0x218);
              in_stack_00000094 = in_stack_00000060;
              in_stack_00000098 = in_stack_00000064;
              in_stack_0000009c = in_stack_00000068;
              in_stack_000000a0 = *(undefined4 *)(unaff_EBP + 0xa0);
              in_stack_000000a4 = *(undefined4 *)(unaff_EBP + 0xa4);
              in_stack_000000a8 = *(undefined4 *)(unaff_EBP + 0xa8);
              in_stack_000000ac = *unaff_ESI;
              in_stack_000000b0 = unaff_ESI[1];
              in_stack_000000b4 = unaff_ESI[2];
              in_stack_000000b8 = 1.0;
              in_stack_000000bc = 0x3f800000;
              in_stack_000000c4 = uStack0000001c;
              in_stack_00000078 = iVar6;
              if (in_stack_00000018 == 0xffffffff) {
                if ((short)in_stack_00000058 != -1) {
                  FUN_004ffde0(in_stack_00000058,in_stack_0000006c);
                }
              }
              else {
                iVar5 = (in_stack_00000018 & 0xffff) * 0xc;
                if (*(short *)(*(int *)(iVar5 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0xb4) == 7) {
                  FUN_0044b5d0();
                }
                if (0.0 < *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x34)) {
                  in_stack_000000b8 =
                       ((*(float *)(unaff_EBP + 0x74) * *(float *)(unaff_EBP + 0x68) +
                        *(float *)(unaff_EBP + 0x6c) * *(float *)(unaff_EBP + 0x78) +
                        *(float *)(unaff_EBP + 0x70) * *(float *)(unaff_EBP + 0x7c)) * 30.0) /
                       *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x34);
                  if (0.0 <= in_stack_000000b8) {
                    if (1.0 < in_stack_000000b8) {
                      in_stack_000000b8 = 1.0;
                    }
                  }
                  else {
                    in_stack_000000b8 = 0.0;
                  }
                }
                if ((*(short *)(unaff_EBP + 0xb4) == 0) &&
                   ('\x0f' < *(char *)(*(int *)(in_stack_00000070 + 8 +
                                               *(int *)(DAT_008603b0 + 0x34)) + 0x501))) {
                  in_stack_000000b8 = 1.5;
                }
                if (*(short *)(*(int *)(iVar5 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0xb4) == 0) {
                  object_apply_damage(&stack0x00000078,in_stack_00000018,0xffffffff,0xffffffff,
                                      0xffffffff,0);
                }
              }
            }
            if (((short)_uStack0000001c != -1) && (FUN_0056f210(), in_stack_0000004c != -1)) {
              fVar1 = *unaff_ESI;
              puVar8 = &stack0x00000078;
              for (iVar5 = 0x15; iVar5 != 0; iVar5 = iVar5 + -1) {
                *puVar8 = 0;
                puVar8 = puVar8 + 1;
              }
              in_stack_000000b0 = -unaff_ESI[1];
              in_stack_0000007c = in_stack_0000007c | 8;
              in_stack_000000b4 = -unaff_ESI[2];
              in_stack_000000a0 = *(undefined4 *)(unaff_EBP + 0xa0);
              in_stack_000000a4 = *(undefined4 *)(unaff_EBP + 0xa4);
              in_stack_000000a8 = *(undefined4 *)(unaff_EBP + 0xa8);
              in_stack_00000094 = *(undefined4 *)(unaff_EBP + 0xa0);
              in_stack_00000098 = *(undefined4 *)(unaff_EBP + 0xa4);
              in_stack_0000009c = *(undefined4 *)(unaff_EBP + 0xa8);
              in_stack_000000c4 = 0xffff;
              in_stack_00000080 = 0xffffffff;
              in_stack_00000084 = 0xffffffff;
              in_stack_00000088 = 0xffff;
              in_stack_00000090 = CONCAT22(in_stack_00000090._2_2_,0xffff);
              in_stack_00000078 = in_stack_0000004c;
              in_stack_000000b8 = 1.0;
              in_stack_000000bc = 0x3f800000;
              in_stack_000000ac = -fVar1;
              object_apply_damage(&stack0x00000078,in_stack_00000144,0xffffffff,0xffffffff,
                                  0xffffffff,0);
            }
            *(undefined1 *)(unaff_EBP + 0x289) = 0;
            return;
          }
          in_stack_00000024 = (float)in_stack_00000014;
          in_stack_00000020 = -2;
          in_stack_00000050 = 5;
        }
        fVar1 = (float)in_stack_00000020;
        in_stack_00000040 =
             *unaff_ESI * 0.8 +
             (fVar1 * in_stack_00000028 + in_stack_00000024 * in_stack_00000034) * 0.1;
        in_stack_00000044 =
             *(float *)(unaff_EBP + 0x240) * 0.8 +
             (fVar1 * in_stack_0000002c + in_stack_00000024 * in_stack_00000038) * 0.1;
        in_stack_00000048 =
             *(float *)(unaff_EBP + 0x244) * 0.8 +
             (fVar1 * in_stack_00000030 + in_stack_00000024 * in_stack_0000003c) * 0.1;
        cVar4 = FUN_00505880(0x1000e9,&stack0x00000060,&stack0x00000040,in_stack_00000144,
                             &stack0x000000f0);
      } while (cVar4 == '\0');
      if (in_stack_000000f0 != 2) {
        if (in_stack_000000f0 == 3) {
          iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_stack_00000128 & 0xffff) * 0xc);
          uVar7 = in_stack_00000128;
          if ((*(short *)(iVar5 + 0xb4) != 2) &&
             (uVar3 = *(uint *)(iVar5 + 0x11c), uVar3 != 0xffffffff)) {
            iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
            uVar7 = uVar3;
          }
          if ((in_stack_00000018 == 0xffffffff) ||
             ((*(short *)(iVar5 + 0xb4) == 0 &&
              (((in_stack_00000054 == 0 && (in_stack_00000104 < in_stack_000000ec)) ||
               (in_stack_00000054 != 0)))))) {
            in_stack_00000054 = *(short *)(iVar5 + 0xb4);
            _uStack0000001c = in_stack_00000124;
            in_stack_000000ec = in_stack_00000104;
            in_stack_00000018 = uVar7;
          }
        }
        goto LAB_0056f82a;
      }
    } while (in_stack_00000018 != 0xffffffff);
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
