// object_damage_apply_line_of_sight
// address 0x4eddb0, size 1294 bytes
// name confidence: 0.5 (Ghidra-recovered name, in-range and self-recursive so kept)
// rewrite confidence: 0.3
// evidence: types/objects.h damage_data (flags 0x04, responsible_object 0x0c, origin 0x28,
// direction 0x34, random_blend 0x40), object (flags 0x10, bounding_center 0xa0, type 0xb4,
// next_object 0x114, first_child_object 0x118, parent_object 0x11c); types/tags.h
// DamageEffect.radius[2] (0x00), .flags (0x0c), .damage_flags (0x1c8, bits 0x01
// does_not_hurt_owner, 0x08 does_not_hurt_friends, 0x400 only_hurts_one_infection_form, 0x1000
// infection_form_pop), .damage_aoe_core_radius (0x1cc); ModelCollisionGeometryFlags bit 0x08
// (passes_area_damage_to_children) at the geometry tag's first dword.
// UNSURE (large parts of this function): collision_test_movement_segment (a 1972-byte raycast/occlusion helper far
// outside this module) is called with 4 or 5 visible arguments depending on call site, and
// Ghidra could not resolve object_get_root_object_index's argument at one call site either
// (it is a 43-byte, argument-less-looking function elsewhere). The perpendicular-sample block
// (the four `local_e0`/`local_ec`-derived offsets from vector3d_build_perpendicular and
// vector3d_cross_product) has outputs Ghidra never shows being written anywhere, meaning the
// real stack layout is a single flat buffer these helpers write into directly that the
// decompiler split into disconnected named locals. This is transcribed as literally as
// possible, including the apparently-unwritten reads, rather than invented.
// register convention: damage_data *param_1 on the stack; datum_index param_2 (target object)
// on the stack; char param_3 (recursion continuation flag) on the stack.
// blam-cc: stack=(dd, target_object_index, continue_flag)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // math module, 0x401990
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // math module, 0x4cd670
extern void vector3d_cross_product(); // math module, 0x4052c0.
    // No prototype is asserted: Ghidra models fewer arguments at this call site than the
    // function really takes, because the missing operands travel in registers it could
    // not source. The empty parameter list is the same convention this module already
    // uses for FUN_00450870 -- it keeps one declaration per symbol without fabricating
    // a signature that contradicts the canonical one.
    // Original note: only one
                                                       // visible argument at this call site
extern real random_real(void); // math module, 0x4019f0

extern int8_t teams_are_enemies(void); // UNSURE: zero visible args; 18 callers, address 0x45bd50
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
    // 0x46fe10, blam-cc: stack -> zoom_table_index, CX -> magnification (every caller passes the difficulty)
extern uint8_t *main_game_globals; // 0x006b0b80 game globals *, +0x0e difficulty
extern uint32_t object_get_root_object_index(uint32_t object_index); // objects module,
    // 0x4f6fb0 (out of range); shown with a stack buffer argument at one call site and with
    // none at another
extern uint8_t collision_test_movement_segment(); // out of range, 0x505880, a BSP ray/segment test.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index,
    int16_t param_4, int16_t material_index, uint32_t param_6); // this module, 0x4ee5e0 // 0x4ee5e0

void object_damage_apply_line_of_sight(damage_data *dd, datum_index target_index, int8_t continue_flag)
{
    object_header *headers = (object_header *)object_data->data;
    object *target;
    Object *target_definition;
    DamageEffect *effect;
    uint8_t has_collision;
    int8_t los_clear = 0;
    int8_t did_recurse_area_damage = 0; // bVar3
    int8_t applied_direct_damage; // bVar10, set only on the direct-hit path
    uint32_t root_index;
    float dx, dy, dz;              // local_c4/c0/bc and local_b8/b4/b0, aliased below
    float basis_a_x, basis_a_y, basis_a_z;   // local_e0/dc/d8 (first perpendicular basis)
    float basis_b_x, basis_b_y, basis_b_z;   // local_ec/e8/e4 (second perpendicular basis)
    float sample_x, sample_y, sample_z;      // local_f8/f4/f0
    real_point3d root_scratch;               // local_d4/d0/cc (from the first collision_test_movement_segment/root call)
    float delta2_x, delta2_y, delta2_z;      // local_ac/a8/a4
    uint8_t collision_result[80];            // local_50
    uint8_t sample_scratch[24];              // local_a0
    float local_88, local_84, local_80; // UNSURE: Ghidra never shows these three floats being
        // written anywhere; they immediately follow sample_scratch[24] on the stack and are
        // presumably filled in place by object_get_root_object_index/collision_test_movement_segment through a
        // pointer this decompile lost. Kept as literal, apparently-unwritten reads rather than
        // invented values.
    int32_t sample_index;
    int32_t remaining;
    float sample_scale;
    real length_fraction;
    datum_index sample_root;
    int8_t sample_ok;

    do {
        target = headers[target_index & 0xffff].data;
        target_definition = (Object *)tag_instances[target->definition_tag & 0xffff].data;
        effect = (DamageEffect *)tag_instances[dd->damage_effect_tag & 0xffff].data;
        has_collision = (uint8_t)(~target->flags & 1);
        did_recurse_area_damage = 0;
        los_clear = 0;
        applied_direct_damage = 0;

        if ((has_collision == 0) ||
            ((1 << (target->type & 0x1f) & _object_mask_unit) == 0) ||
            (effect->damage_aoe_core_radius <= 0.0001f)) {
            uint32_t walk;

            root_index = 0xffffffff;
            for (walk = target_index; walk != 0xffffffff; walk = headers[walk & 0xffff].data->parent_object) {
                root_index = walk;
            }

            dx = target->bounding_center.x - dd->origin.x;
            dy = target->bounding_center.y - dd->origin.y;
            dz = target->bounding_center.z - dd->origin.z;
            los_clear = collision_test_movement_segment(0xc221, &dd->origin, (real_vector3d *)&dx, root_index, collision_result);
        } else {
            real_vector3d dir;
            real_vector3d basis_a;
            real_vector3d basis_b;

            dx = target->bounding_center.x - dd->origin.x;
            los_clear = 1;
            dy = target->bounding_center.y - dd->origin.y;
            dz = target->bounding_center.z - dd->origin.z;

            dir.i = dx; dir.j = dy; dir.k = dz;
            vector3d_build_perpendicular(&basis_a, &dir); // UNSURE: output target inferred
            vector3d_normalize_with_length(&basis_a);
            vector3d_cross_product(&dir); // UNSURE: only one visible argument
            vector3d_normalize_with_length(&basis_b);
            basis_a_x = basis_a.i; basis_a_y = basis_a.j; basis_a_z = basis_a.k;
            basis_b_x = basis_b.i; basis_b_y = basis_b.j; basis_b_z = basis_b.k;

            sample_index = 0;
            remaining = 4;
            do {
                switch (sample_index) {
                case 0:
                    sample_scale = effect->damage_aoe_core_radius;
                    sample_x = basis_b_x * sample_scale;
                    sample_y = basis_b_y * sample_scale;
                    sample_z = basis_b_z;
                    sample_z = sample_z * sample_scale;
                    break;
                case 1:
                    sample_scale = -effect->damage_aoe_core_radius;
                    sample_x = basis_b_x * sample_scale;
                    sample_y = basis_b_y * sample_scale;
                    sample_z = basis_b_z;
                    sample_z = sample_z * sample_scale;
                    break;
                case 2:
                    sample_scale = effect->damage_aoe_core_radius;
                    sample_x = basis_a_x * sample_scale;
                    sample_y = basis_a_y * sample_scale;
                    sample_z = basis_a_z;
                    sample_z = sample_z * sample_scale;
                    break;
                case 3:
                default:
                    sample_scale = -effect->damage_aoe_core_radius;
                    sample_x = basis_a_x * sample_scale;
                    sample_y = basis_a_y * sample_scale;
                    sample_z = basis_a_z;
                    sample_z = sample_z * sample_scale;
                    break;
                }

                // PHASE-4 REVIEW: Ghidra attributes a `push ecx` to this call, but that push
                // is collision_test_movement_segment's last argument. objdump 0x4edf3f and 0x4edf75 both load
                // ECX -- 0x4f6fb0's only input -- from the same stack slot, which is this
                // function's own target_index parameter.
                sample_root = object_get_root_object_index(target_index);
                collision_test_movement_segment(0xc221, &dd->origin, (real_vector3d *)&sample_x, sample_root, 0);
                root_scratch.x = local_88; // local_d4 = local_88
                root_scratch.y = local_84; // local_d0 = local_84
                root_scratch.z = local_80; // local_cc = local_80
                sample_root = object_get_root_object_index(target_index);
                delta2_x = target->bounding_center.x - root_scratch.x;
                delta2_y = target->bounding_center.y - root_scratch.y;
                delta2_z = target->bounding_center.z - root_scratch.z;
                sample_ok = collision_test_movement_segment(0xc221, (real_point3d *)&root_scratch, (real_vector3d *)&delta2_x,
                    sample_root, sample_scratch);
                if (sample_ok == 0) {
                    los_clear = 0;
                }

                sample_index = sample_index + 1;
                remaining = remaining - 1;
            } while (remaining != 0);
        }

        if (los_clear != 0) {
            has_collision = 0;
        }

        if (((effect->damage_flags & 1) != 0) && (target_index == dd->responsible_object)) {
            has_collision = 0; // does_not_hurt_owner
        }

        if (((effect->damage_flags & 8) == 0) || (teams_are_enemies() != 0)) { // does_not_hurt_friends
            if ((has_collision != 0) && ((effect->damage_flags & 0x1000) != 0)) { // infection_form_pop
                has_collision = 0;
                if (((1 << (target->type & 0x1f) & _object_mask_unit) != 0) &&
                    ((((Unit *)tag_instances[target->definition_tag & 0xffff].data)->unit_flags & 0x80000) != 0) && // inconsequential
                    (target_index != dd->responsible_object)) {
                    real difficulty = weapon_get_zoom_fov(8, *(int16_t *)(main_game_globals + 0x0e));

                    has_collision = 1;
                    if (((0.0f < difficulty) || ((effect->damage_flags & 0x400) != 0)) && // only_hurts_one_infection_form
                        ((dd->flags & 0x40) != 0)) {
                        has_collision = 0;
                    }
                    if ((0.0f < difficulty) && (random_real() < difficulty * 0.25f)) {
                        has_collision = 0;
                    }
                    did_recurse_area_damage = 1;
                }
            }
        } else {
            has_collision = 0;
        }

        dd->flags |= 1;

        if (has_collision != 0) {
            real_vector3d direction;

            dd->direction.i = target->bounding_center.x - dd->origin.x;
            dd->direction.j = target->bounding_center.y - dd->origin.y;
            dd->direction.k = target->bounding_center.z - dd->origin.z;
            direction = dd->direction;
            length_fraction = vector3d_normalize_with_length(&direction);
            dd->direction = direction;

            if (effect->radius[1] - effect->radius[0] <= 0.0f) {
                length_fraction = 1.0f;
            } else {
                length_fraction = 1.0f - (length_fraction - effect->radius[0]) / (effect->radius[1] - effect->radius[0]);
                if (0.0f <= length_fraction) {
                    if (1.0f < length_fraction) {
                        length_fraction = 1.0f;
                    }
                } else {
                    length_fraction = 0.0f;
                }
            }

            if ((effect->flags & 1) == 0) {
                dd->random_blend = length_fraction;
            }

            applied_direct_damage = 0.0f < length_fraction;
            if (applied_direct_damage) {
                object_apply_damage(dd, target_index, 0xffffffff, 0xffffffff, 0xffffffff, 0);
            }

            if ((target_definition->collision_model.tag_id.index != 0xffff) &&
                ((*(uint8_t *)tag_instances[target_definition->collision_model.tag_id.index].data & 8) != 0) &&
                (target->first_child_object != (datum_index)0xffffffff)) {
                object_damage_apply_line_of_sight(dd, target->first_child_object, 1);
            }
        }

        if (did_recurse_area_damage && ((has_collision == 0) || applied_direct_damage)) {
            dd->flags |= 0x40;
        }

        if ((continue_flag == 0) || (target_index = target->next_object, target_index == (datum_index)0xffffffff)) {
            return;
        }
        continue_flag = 1;
    } while (1);
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
