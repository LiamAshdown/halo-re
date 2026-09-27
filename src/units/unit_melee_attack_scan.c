// unit_melee_attack_scan  (Ghidra: already named unit_melee_attack_scan)
// address 0x56f550, size 1839 bytes
// name confidence: 0.5 (functions.md summary matches the code)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x56f550..0x56fc7e (the whole function; the old header's 681 bytes stopped
//   mid-loop and the tail was carved out as "unit_can_see_point" 0x56f800, now marked a FRAGMENT).
//   Stack: unit_index. From the "primary eye"-adjacent marker (0x66bfa0) a 5x5 cone of segments
//   aim*0.8 + (row*perp + col*(aim x perp))*0.1 is cast (0x505880, mask 0x1000e9). A structure hit
//   (kind 2) before any object hit remembers the material (+0x34) and, for a breakable surface
//   (+0x4c bit 8), its index (+0x4d) and surface (+0x44); an object hit (kind 3, non-weapons resolve
//   to their parent) keeps the first, then prefers bipeds, the nearest biped (+0x14) winning. The
//   damage effect is the wielded weapon's (+0x3a0, secondary +0x3b0) or the unit tag's (+0x294). A
//   struck vehicle not network-role 1 gets an impulse of aim * its acceleration scale * 0.035
//   (0x571cb0); the damage record (flags 1, player +0x218, team +0xb8, marker epicentre, centre
//   origin, aim direction, material) breaks a surface (0x4ffde0), tells a machine (0x44b5d0) and
//   damages a biped, scaled by the attacker's forward speed * 30 / globals melee speed (clamped
//   0..1, 1.5 once a biped has been grounded more than 15 ticks). A material then plays its hit
//   effect (0x56f210) and the secondary effect pushes the attacker back (flags 8, -aim). Clears
//   the melee state (+0x289).
// blam-cc: stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"
#include <string.h>

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *globals_tag_data;    // 0x00746fa0
extern char s_primary_eye_marker[];  // 0x0066bfa0

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670, ECX, EDX (returns out in EAX)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id,
                                             datum_index object_index); // 0x56f210, AX, ECX, EDX
extern void breakable_surface_apply_damage(damage_data *damage, int32_t surface_index,
                                           int32_t collision_surface_index); // 0x4ffde0, EBX, stack
extern void device_machine_melee_attacked(uint32_t object_index); // 0x44b5d0, ECX
extern void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse); // 0x571cb0, ECX, EAX

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void unit_melee_attack_scan(uint32_t unit_index)
{
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)obj);
    real_vector3d *aim = (real_vector3d *)(obj + 0x23c);
    object_marker marker;
    real_point3d origin;
    real_vector3d perp;
    real_vector3d side;
    uint32_t best_object = 0xffffffff;          // [esp+0x18]
    int32_t material = -1;                      // [esp+0x1c]
    int16_t best_type = 0;                      // [esp+0x54]
    float best_fraction = 0.0f;                 // [esp+0xec]
    uint32_t breakable_index = 0xffffffff;      // [esp+0x58]
    int32_t breakable_surface = 0;              // [esp+0x6c]
    datum_index secondary_effect = 0xffffffff;  // [esp+0x4c]
    datum_index damage_effect;
    int32_t row;
    int32_t col;

    object_get_node_local_transform(unit_index, s_primary_eye_marker, &marker, 1);
    origin = marker.node_transform.position;
    vector3d_build_perpendicular(&perp, aim);
    vector3d_normalize_with_length(&perp);
    side.i = aim->j * perp.k - aim->k * perp.j;
    side.j = aim->k * perp.i - aim->i * perp.k;
    side.k = aim->i * perp.j - aim->j * perp.i;

    for (row = -2; row <= 2; row++) {
        float rowf = (float)row;

        for (col = -2; col <= 2; col++) {
            float colf = (float)col;
            real_vector3d delta;
            collision_result hit;

            delta.i = (rowf * perp.i + colf * side.i) * 0.1f + aim->i * 0.8f;
            delta.j = (rowf * perp.j + colf * side.j) * 0.1f + aim->j * 0.8f;
            delta.k = (rowf * perp.k + colf * side.k) * 0.1f + aim->k * 0.8f;
            if (!collision_test_movement_segment(0x1000e9, &origin, &delta, unit_index, &hit)) {
                continue;
            }
            if (*(int16_t *)&hit == 2) {
                // 0x56f7fa: a structure surface, only while no object has been hit
                if (best_object == 0xffffffff) {
                    material = *(int32_t *)&hit.material_type;
                    if (hit.surface_flags & 8) {
                        breakable_index = (breakable_index & 0xffff0000u) | hit.breakable_surface_index;
                        breakable_surface = hit.surface_index;
                    }
                }
            } else if (*(int16_t *)&hit == 3) {
                // 0x56f74f: an object; anything but a weapon resolves to its parent
                uint32_t candidate = hit.object_index;
                uint8_t *cand = OBJECT_DATA(candidate);
                int16_t type;

                if (*(int16_t *)(cand + 0xb4) != 2 && *(datum_index *)(cand + 0x11c) != k_datum_index_none) {
                    candidate = *(datum_index *)(cand + 0x11c);
                    cand = OBJECT_DATA(candidate);
                }
                type = *(int16_t *)(cand + 0xb4);
                if (best_object != 0xffffffff) {
                    if (type != 0) {
                        continue;
                    }
                    if (best_type == 0 && !(best_fraction > hit.t)) {
                        continue;
                    }
                }
                best_object = candidate;
                best_type = type;
                material = *(int32_t *)&hit.material_type;
                best_fraction = hit.t;
            }
        }
    }

    // 0x56f85a: the weapon's melee damage, else the unit's
    damage_effect = 0xffffffff;
    {
        int16_t weapon_slot = *(int16_t *)(obj + 0x2f2);

        if (weapon_slot != -1) {
            datum_index weapon_index = *(datum_index *)(obj + 0x2f8 + weapon_slot * 4);

            if (weapon_index != k_datum_index_none) {
                uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon_index));

                damage_effect = *(datum_index *)(weapon_tag + 0x3a0);
                secondary_effect = *(datum_index *)(weapon_tag + 0x3b0);
            }
        }
    }
    if (damage_effect == 0xffffffff) {
        damage_effect = *(datum_index *)(unit_tag + 0x294);
    }

    if (best_object != 0xffffffff) {
        uint8_t *best = OBJECT_DATA(best_object);

        if (*(int16_t *)(best + 0xb4) == 1 && *(int32_t *)(best + 0x4) != 1) {
            float scale = *(float *)(TAG_DATA(*(datum_index *)best) + 0x20) * 0.035f;

            side.i = scale * aim->i;
            side.j = scale * aim->j;
            side.k = scale * aim->k;
            unit_apply_impulse_to_seat(best_object, &side);
        }
    }

    if (damage_effect != 0xffffffff) {
        damage_data dd;

        memset(&dd, 0, sizeof(dd));
        dd.flags |= 1;
        dd.damage_effect_tag = damage_effect;
        dd.responsible_player = *(datum_index *)(obj + 0x218);
        dd.responsible_object = unit_index;
        dd.team_index = *(int16_t *)(obj + 0xb8);
        dd.location_leaf_index = *(int32_t *)(obj + 0x98);
        *(int32_t *)&dd.location_cluster_index = *(int32_t *)(obj + 0x9c);
        dd.epicentre = origin;
        dd.origin = *(real_point3d *)(obj + 0xa0);
        dd.direction = *aim;
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.material_type = (int16_t)material;

        if (best_object == 0xffffffff) {
            if ((int16_t)breakable_index != -1) {
                breakable_surface_apply_damage(&dd, (int32_t)breakable_index, breakable_surface);
            }
        } else {
            float speed_scale = *(float *)(*(uint8_t **)(globals_tag_data + 0x174) + 0x34);

            if (*(int16_t *)(OBJECT_DATA(best_object) + 0xb4) == 7) {
                device_machine_melee_attacked(best_object);
            }
            if (speed_scale > 0.0f) {
                float f = (*(float *)(obj + 0x70) * *(float *)(obj + 0x7c) +
                           *(float *)(obj + 0x6c) * *(float *)(obj + 0x78) +
                           *(float *)(obj + 0x74) * *(float *)(obj + 0x68)) * 30.0f / speed_scale;

                if (f < 0.0f) {
                    f = 0.0f;
                } else if (f > 1.0f) {
                    f = 1.0f;
                }
                dd.random_blend = f;
            }
            if (*(int16_t *)(obj + 0xb4) == 0 && *(int8_t *)(obj + 0x501) > 0x0f) {
                dd.random_blend = 1.5f;
            }
            if (*(int16_t *)(OBJECT_DATA(best_object) + 0xb4) == 0) {
                object_apply_damage(&dd, best_object, -1, -1, -1, 0);
            }
        }
    }

    // 0x56fb65
    if ((int16_t)material != -1) {
        unit_trigger_material_hit_effect((int16_t)material, damage_effect, unit_index);
        if (secondary_effect != 0xffffffff) {
            damage_data dd;

            memset(&dd, 0, sizeof(dd));
            dd.flags |= 8;
            dd.damage_effect_tag = secondary_effect;
            dd.responsible_player = k_datum_index_none;
            dd.responsible_object = k_datum_index_none;
            dd.team_index = -1;
            dd.location_cluster_index = -1;
            dd.epicentre = *(real_point3d *)(obj + 0xa0);
            dd.origin = *(real_point3d *)(obj + 0xa0);
            dd.direction.i = -aim->i;
            dd.direction.j = -aim->j;
            dd.direction.k = -aim->k;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            dd.material_type = -1;
            object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
        }
    }
    obj[0x289] = 0;
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
