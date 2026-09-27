// object_damage_apply_line_of_sight
// address 0x4eddb0, size 1294 bytes
// name confidence: 0.5 (Ghidra-recovered name, in-range and self-recursive so kept)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4eddb0..0x4ee2bd. Stack: (damage, object, siblings too). The draft called the cross
//   product with one argument (a crash on the first explosion that reached a visible biped or vehicle) and the
//   segment tests unprototyped. For each object (and its siblings when asked): a visible unit is hit when any of
//   four points offset by the effect's core radius (+0x1cc) around the line from the origin sees its centre; other
//   objects need a clear line from the origin (0x505880, mask 0xc221, ignoring the object's root). Effect flags
//   +0x1c8: 1 spares the causer, 8 hurts only enemies, 0x1000 lets difficulty (0x46fe10 table 8) spare player
//   bipeds. The damage falls off from +0x0 to +0x4 with distance into random_blend (unless +0xc bit 1), is applied
//   (0x4ee5e0) when positive, and recurses into children when the object's collision model says so; flag 0x40 marks
//   a spared player.
// blam-cc: stack -> damage, object_index, recurse_siblings

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *main_game_globals;  // 0x006b0b80, +0x0e the difficulty

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670, ECX, EDX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern real random_real(void); // 0x4019f0
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, stack, CX
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index, int16_t region_index,
    int16_t material_index, uint32_t plane); // 0x4ee5e0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void object_damage_apply_line_of_sight(damage_data *dd, datum_index target_index, int8_t continue_flag)
{
    uint8_t *effect = TAG_DATA(dd->damage_effect_tag);         // [esp+0x18]
    real_point3d *origin = &dd->origin;                        // edi

    for (;;) {
        uint8_t *target = OBJECT_DATA(target_index);            // esi
        uint8_t *target_tag = TAG_DATA(*(datum_index *)target);  // [esp+0x50]
        uint8_t apply = (uint8_t)(~target[0x10] & 1);           // bl
        uint8_t applied = 0;                                    // [esp+0x15]
        uint8_t spared_player = 0;                              // [esp+0x16]
        uint8_t blocked;
        uint32_t flags;

        if (apply && ((1u << (target[0xb4] & 0x1f)) & 3) && *(float *)(effect + 0x1cc) > 9.999999747378752e-05f) {
            // 0x4ede58: four points around the line, a core radius out
            real_vector3d to_center;        // [esp+0x54]
            real_vector3d side_a;           // [esp+0x2c]
            real_vector3d side_b;           // [esp+0x38]
            int32_t i;

            blocked = 1;
            to_center.i = *(float *)(target + 0xa0) - origin->x;
            to_center.j = *(float *)(target + 0xa4) - origin->y;
            to_center.k = *(float *)(target + 0xa8) - origin->z;
            vector3d_build_perpendicular(&side_a, &to_center);
            vector3d_normalize_with_length(&side_a);
            vector3d_cross_product(&side_b, &side_a, &to_center);
            vector3d_normalize_with_length(&side_b);
            for (i = 0; i < 4; i++) {
                real radius = *(float *)(effect + 0x1cc);
                real_vector3d *side = i < 2 ? &side_a : &side_b;
                real_vector3d offset;       // [esp+0x20]
                real_point3d sample;        // [esp+0x44]
                real_vector3d back;         // [esp+0x6c]
                collision_result hit;       // [esp+0x78]

                if (i & 1) {
                    radius = -radius;
                }
                offset.i = side->i * radius;
                offset.j = side->j * radius;
                offset.k = side->k * radius;
                collision_test_movement_segment(0xc221, origin, &offset, object_get_root_object_index(target_index), &hit);
                sample = hit.point;
                back.i = *(float *)(target + 0xa0) - sample.x;
                back.j = *(float *)(target + 0xa4) - sample.y;
                back.k = *(float *)(target + 0xa8) - sample.z;
                if (!collision_test_movement_segment(0xc221, &sample, &back, object_get_root_object_index(target_index), &hit)) {
                    blocked = 0;
                }
            }
        } else {
            // 0x4edffe: one line from the origin to the centre
            datum_index root = k_datum_index_none;
            datum_index walk = target_index;
            real_vector3d to_center;        // [esp+0x64]
            collision_result hit;           // [esp+0xc8]

            while (walk != k_datum_index_none) {
                root = walk;
                walk = *(datum_index *)(OBJECT_DATA(walk) + 0x11c);
            }
            to_center.i = *(float *)(target + 0xa0) - origin->x;
            to_center.j = *(float *)(target + 0xa4) - origin->y;
            to_center.k = *(float *)(target + 0xa8) - origin->z;
            blocked = collision_test_movement_segment(0xc221, origin, &to_center, root, &hit);
        }
        if (blocked) {
            apply = 0;
        }

        // 0x4ee074: who the effect may hurt
        flags = *(uint32_t *)(effect + 0x1c8);
        if ((flags & 1) && target_index == dd->responsible_object) {
            apply = 0;
        }
        if ((flags & 8) && !teams_are_enemies(dd->team_index, *(int16_t *)(target + 0xb8))) {
            apply = 0;
        } else if (apply && (flags & 0x1000)) {
            apply = 0;
            if (((1u << (target[0xb4] & 0x1f)) & 3) &&
                (*(uint32_t *)(TAG_DATA(*(datum_index *)target) + 0x17c) & 0x80000) &&
                target_index != dd->responsible_object) {
                real scale = weapon_get_zoom_fov(8, *(int16_t *)(main_game_globals + 0xe));

                apply = 1;
                if ((scale > 0.0f || (flags & 0x400)) && (dd->flags & 0x40)) {
                    apply = 0;
                }
                if (scale > 0.0f && scale * 0.25f > random_real()) {
                    apply = 0;
                }
                spared_player = 1;
            }
        }
        dd->flags |= 1;

        if (apply) {
            // 0x4ee189: direction and falloff
            real distance;
            real blend;
            real range;

            dd->direction.i = *(float *)(target + 0xa0) - origin->x;
            dd->direction.j = *(float *)(target + 0xa4) - origin->y;
            dd->direction.k = *(float *)(target + 0xa8) - origin->z;
            distance = vector3d_normalize_with_length(&dd->direction);
            range = *(float *)(effect + 0x4) - *(float *)(effect + 0x0);
            if (range > 0.0f) {
                blend = 1.0f - (distance - *(float *)(effect + 0x0)) / range;
                if (!(blend >= 0.0f)) {
                    blend = 0.0f;
                } else if (!(blend <= 1.0f)) {
                    blend = 1.0f;
                }
            } else {
                blend = 1.0f;
            }
            if (!(effect[0xc] & 1)) {
                dd->random_blend = blend;
            }
            if (blend > 0.0f) {
                object_apply_damage(dd, target_index, -1, -1, -1, 0);
                applied = 1;
            }
            {
                datum_index model = *(datum_index *)(target_tag + 0x7c);

                if (model != k_datum_index_none && (*TAG_DATA(model) & 8) &&
                    *(datum_index *)(target + 0x118) != k_datum_index_none) {
                    object_damage_apply_line_of_sight(dd, *(datum_index *)(target + 0x118), 1);
                }
            }
        }
        if (spared_player && (!apply || !applied)) {
            dd->flags |= 0x40;
        }
        if (!continue_flag || *(datum_index *)(target + 0x114) == k_datum_index_none) {
            return;
        }
        continue_flag = 1;
        target_index = *(datum_index *)(target + 0x114);
    }
}

#if 0
Original Ghidra decompilation (0x4eddb0):

void object_damage_apply_line_of_sight(uint *param_1,uint param_2,char param_3)

{
  uint *puVar1;
  float *pfVar2;
  bool bVar3;
  uint uVar4;
  uint uVar5;
  char cVar6;
  undefined4 uVar7;
  byte bVar8;
  int iVar9;
  bool bVar10;
  float10 fVar11;
  float fVar12;
  char local_101;
  int local_fc;
  float local_f8;
  float local_f4;
  float local_f0;
  float local_ec;
  float local_e8;
  float local_e4;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  float local_d0;
  float local_cc;
  int local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  undefined1 local_a0 [24];
  float local_88;
  float local_84;
  float local_80;
  undefined1 local_50 [80];

  do {
    puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    local_c8 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    pfVar2 = *(float **)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    bVar8 = ~(byte)puVar1[4] & 1;
    bVar10 = false;
    bVar3 = false;
    if (((bVar8 == 0) || ((1 << ((byte)puVar1[0x2d] & 0x1f) & 3U) == 0)) || (pfVar2[0x73] <= 0.0001)
       ) {
      uVar4 = 0xffffffff;
      for (uVar5 = param_2; uVar5 != 0xffffffff;
          uVar5 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) +
                           0x11c)) {
        uVar4 = uVar5;
      }
      local_b8 = (float)puVar1[0x28] - (float)param_1[10];
      local_b4 = (float)puVar1[0x29] - (float)param_1[0xb];
      local_b0 = (float)puVar1[0x2a] - (float)param_1[0xc];
      local_101 = FUN_00505880(0xc221,param_1 + 10,&local_b8,uVar4,local_50);
    }
    else {
      local_c4 = (float)puVar1[0x28] - (float)param_1[10];
      local_101 = '\x01';
      local_c0 = (float)puVar1[0x29] - (float)param_1[0xb];
      local_bc = (float)puVar1[0x2a] - (float)param_1[0xc];
      vector3d_build_perpendicular();
      vector3d_normalize_with_length();
      vector3d_cross_product(&local_c4);
      vector3d_normalize_with_length();
      iVar9 = 0;
      local_fc = 4;
      do {
        switch(iVar9) {
        case 0:
          fVar12 = pfVar2[0x73];
          break;
        case 1:
          fVar12 = -pfVar2[0x73];
          break;
        case 2:
          fVar12 = pfVar2[0x73];
          goto LAB_004edf1a;
        case 3:
          fVar12 = -pfVar2[0x73];
LAB_004edf1a:
          local_f8 = local_e0 * fVar12;
          local_f4 = local_dc * fVar12;
          local_f0 = local_d8;
          goto LAB_004edf32;
        default:
          goto switchD_004edec9_default;
        }
        local_f8 = local_ec * fVar12;
        local_f4 = local_e8 * fVar12;
        local_f0 = local_e4;
LAB_004edf32:
        local_f0 = local_f0 * fVar12;
switchD_004edec9_default:
        uVar7 = object_get_root_object_index(local_a0);
        FUN_00505880(0xc221,param_1 + 10,&local_f8,uVar7);
        local_d0 = local_84;
        local_d4 = local_88;
        local_cc = local_80;
        uVar7 = object_get_root_object_index();
        local_ac = (float)puVar1[0x28] - local_d4;
        local_a8 = (float)puVar1[0x29] - local_d0;
        local_a4 = (float)puVar1[0x2a] - local_cc;
        cVar6 = FUN_00505880(0xc221,&local_d4,&local_ac,uVar7,local_a0);
        if (cVar6 == '\0') {
          local_101 = '\0';
        }
        iVar9 = iVar9 + 1;
        local_fc = local_fc + -1;
      } while (local_fc != 0);
    }
    if (local_101 != '\0') {
      bVar8 = 0;
    }
    if ((((uint)pfVar2[0x72] & 1) != 0) && (param_2 == param_1[3])) {
      bVar8 = 0;
    }
    if ((((uint)pfVar2[0x72] & 8) == 0) || (cVar6 = FUN_0045bd50(), cVar6 != '\0')) {
      if ((bVar8 != 0) && (((uint)pfVar2[0x72] & 0x1000) != 0)) {
        bVar8 = 0;
        if ((((1 << ((byte)puVar1[0x2d] & 0x1f) & 3U) != 0) &&
            (bVar8 = 0,
            (*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x80000)
            != 0)) && (param_2 != param_1[3])) {
          fVar11 = (float10)FUN_0046fe10(8);
          bVar8 = 1;
          if ((((float10)0.0 < fVar11) || (((uint)pfVar2[0x72] & 0x400) != 0)) &&
             ((param_1[1] & 0x40) != 0)) {
            bVar8 = 0;
          }
          if (((float10)0.0 < fVar11) && (fVar12 = random_real(), fVar12 < (float)fVar11 * 0.25)) {
            bVar8 = 0;
          }
          bVar3 = true;
        }
      }
    }
    else {
      bVar8 = 0;
    }
    param_1[1] = param_1[1] | 1;
    if (bVar8 != 0) {
      param_1[0xd] = (uint)((float)puVar1[0x28] - (float)param_1[10]);
      param_1[0xe] = (uint)((float)puVar1[0x29] - (float)param_1[0xb]);
      param_1[0xf] = (uint)((float)puVar1[0x2a] - (float)param_1[0xc]);
      fVar11 = (float10)vector3d_normalize_with_length();
      if ((float10)pfVar2[1] - (float10)*pfVar2 <= (float10)0.0) {
LAB_004ee201:
        fVar11 = (float10)1.0;
      }
      else {
        fVar11 = (float10)1.0 -
                 (fVar11 - (float10)*pfVar2) / ((float10)pfVar2[1] - (float10)*pfVar2);
        if ((float10)0.0 <= fVar11) {
          if ((float10)1.0 < fVar11) goto LAB_004ee201;
        }
        else {
          fVar11 = (float10)0.0;
        }
      }
      if (((uint)pfVar2[3] & 1) == 0) {
        param_1[0x10] = (uint)(float)fVar11;
      }
      bVar10 = (float10)0.0 < fVar11;
      if (bVar10) {
        object_apply_damage(param_1,param_2,0xffffffff,0xffffffff,0xffffffff,0);
      }
      if (((*(uint *)(local_c8 + 0x7c) != 0xffffffff) &&
          ((**(byte **)((*(uint *)(local_c8 + 0x7c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 8) !=
           0)) && (puVar1[0x46] != 0xffffffff)) {
        object_damage_apply_line_of_sight(param_1,puVar1[0x46],1);
      }
    }
    if ((bVar3) && ((bVar8 == 0 || (bVar10)))) {
      param_1[1] = param_1[1] | 0x40;
    }
    if ((param_3 == '\0') || (param_2 = puVar1[0x45], param_2 == 0xffffffff)) {
      return;
    }
    param_3 = '\x01';
  } while( true );
}
#endif
