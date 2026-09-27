// player_find_placement_position  (Ghidra: FUN_004757b0)
// address 0x4757b0, size 1181 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// REWRITTEN from objdump 0x4757b0..0x475c58. Stack: (player, target object, point). Places the player's unit:
//   with no target (or a target that is its own root) around the point (unit_find_placement_position, radius 2 x
//   pill); otherwise on a ring around the target's root -- 3 x the unit's collision radius + the root's bounding
//   radius, facing away from the root's flat velocity (else its forward, or its up when the forward points up)
//   -- at the 9 offsets of 0x6574c0, each retried from 8 random jitters (0x6b7af4). A placement inside a trigger
//   volume of the current BSP counts as a failure; failures release the unit (0x4760b0). On success with a
//   target the unit stops, copies the target's facing (+0x224 / +0x230 / +0x254; a biped target also its +0x4d3 /
//   +0x4d4), gets matching look angles and the globals' teleport effect. The draft lacked the point argument.
// blam-cc: stack -> player_index, target_object, point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "effects.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *player_data;      // 0x0087a480
extern Scenario *global_scenario;    // 0x00746f8c
extern uint8_t *global_globals_bytes; // 0x00746fa0 (+0x174 -> +0xc4 the teleport effect)
extern uint8_t *local_player_globals_bytes; // 0x0087a478
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern real_vector3d *global_zero_vector3d_pointer; // 0x00696714
extern uint32_t random_seed_global; // 0x00719cd0
extern real_point3d *random_point_table; // 0x006b7af4, a POINTER
extern int16_t random_point_table_count; // 0x006b7af8
extern real_point3d player_placement_ring[9]; // 0x006574c0

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, EAX, ECX, stack
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, EAX, EDX, stack (EAX still = out after it)
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point); // 0x53f020, EAX, ECX
extern uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object,
    real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius,
    uint32_t object_index_a, real_vector3d *reference_direction); // 0x55a500, stack, EDX
extern void player_release_unit_and_reset(uint32_t player_index, int32_t previous_unit_override); // 0x4760b0, EAX, stack
extern void game_engine_compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index); // 0x470d80, EAX, CX
extern void game_engine_build_visible_cluster_bitmask(uint32_t *out_bitmask, uint8_t local_players_only); // 0x4782a0
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source); // 0x4507a0, EAX, ECX, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

uint8_t player_find_placement_position(uint32_t player_index, datum_index target_object, real_point3d *point)
{
    uint8_t *player = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;   // [ebp-0x14]
    uint32_t unit_index = *(datum_index *)(player + 0x34);                              // [ebp-0xc]
    uint8_t *unit = OBJECT_DATA(unit_index);                                             // [ebp-0x4]
    uint8_t placed = 0;                                                                  // bl
    real_vector3d facing;                                                                // [ebp-0x24]

    if (target_object == k_datum_index_none ||
        object_get_root_object_index(target_object) == target_object) {
        placed = (uint8_t)unit_find_placement_position(unit_index, target_object, 0, 2.0f, 0, 0, 1, 0,
            (real_vector3d *)point);
    } else {
        uint32_t root = object_get_root_object_index(target_object);
        uint8_t *root_object = OBJECT_DATA(root);
        float collision_radius;                     // [ebp-0x8]
        real_matrix4x3 ring;                        // [ebp-0x78]
        int16_t i;

        target_object = root;
        facing = *(real_vector3d *)(root_object + 0x68);
        if (!(facing.j * facing.j + facing.i * facing.i > 0.0f)) {
            if (*(float *)(root_object + 0x7c) >= 0.70710677f) {
                facing = *(real_vector3d *)(root_object + 0x80);
            } else {
                facing = *(real_vector3d *)(root_object + 0x74);
            }
        }
        collision_radius = *(float *)((uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data + 0x42c);
        facing.k = 0.0f;
        facing.i = -facing.i;
        facing.j = -facing.j;
        vector3d_normalize_with_length(&facing);
        matrix4x3_from_forward_up(global_up3d_pointer, &facing, &ring);
        ring.position = *(real_point3d *)(root_object + 0xa0);
        ring.scale = collision_radius * 3.0f + *(float *)(root_object + 0xac);
        for (i = 0; !placed && (uint16_t)i < 9; i++) {
            real_point3d spot;                      // [ebp-0x30]
            int16_t attempt;

            matrix4x3_transform_point(&spot, &player_placement_ring[i], &ring);
            placed = (uint8_t)unit_find_placement_position(unit_index, root, 0, 2.0f, 0, 0, 1, 0,
                (real_vector3d *)&spot);
            for (attempt = 0; !placed && attempt < 8; attempt++) {
                real_point3d jittered;              // [ebp-0x3c]
                int16_t index;

                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                index = (int16_t)(((random_seed_global >> 16) * (int32_t)random_point_table_count) >> 16);
                facing = *(real_vector3d *)&random_point_table[index];
                jittered.x = facing.i * collision_radius + spot.x;
                jittered.y = facing.j * collision_radius + spot.y;
                jittered.z = facing.k * collision_radius + spot.z;
                placed = (uint8_t)unit_find_placement_position(unit_index, root, 0, 2.0f, 0, 0, 1, 0,
                    (real_vector3d *)&jittered);
            }
        }
    }
    *(int16_t *)(player + 0x3c) = -1;
    if (placed) {
        uint8_t *volumes = *(uint8_t **)((uint8_t *)global_scenario + 0x3a0);
        int16_t v;

        for (v = 0; (int32_t)v < *(int32_t *)((uint8_t *)global_scenario + 0x39c); v++) {
            uint8_t *volume = *(uint8_t **)((uint8_t *)global_scenario + 0x3a0) + v * 8;
            datum_index player_unit = *(datum_index *)(player + 0x34);

            (void)volumes;
            if (*(int16_t *)(volume + 0x2) == global_structure_bsp_index && player_unit != k_datum_index_none &&
                scenario_trigger_volume_contains_point(*(int16_t *)volume, (real_point3d *)(OBJECT_DATA(player_unit) + 0xa0))) {
                placed = 0;
                break;
            }
        }
    }
    if (!placed) {
        player_release_unit_and_reset(player_index, (int32_t)target_object);
        return 0;
    }
    *(real_vector3d *)(unit + 0x68) = *global_zero_vector3d_pointer;
    if (target_object == k_datum_index_none) {
        return placed;
    }
    facing = *(real_vector3d *)(OBJECT_DATA(target_object) + 0x74);
    {
        // 0x475b05: an inline object datum_try_and_get of the target; a biped target lends its +0x4d3 / +0x4d4
        int16_t index = (int16_t)target_object;
        int16_t salt = (int16_t)(target_object >> 16);

        if (index >= 0 && index < *(int16_t *)((uint8_t *)object_data + 0x20)) {
            uint8_t *header = (uint8_t *)object_data->data + *(int16_t *)((uint8_t *)object_data + 0x22) * index;

            if (*(int16_t *)header != 0 && (salt == 0 || *(int16_t *)header == salt) && header[0x3] == 0) {
                uint8_t *target = *(uint8_t **)(header + 0x8);

                if (target != 0 && *(int32_t *)(target + 0x4d4) != -1) {
                    *(int32_t *)(unit + 0x4d4) = *(int32_t *)(target + 0x4d4);
                    unit[0x4d3] = target[0x4d3];
                }
            }
        }
    }
    *(real_vector3d *)(unit + 0x224) = facing;
    *(real_vector3d *)(unit + 0x230) = facing;
    *(real_vector3d *)(unit + 0x254) = facing;
    if (*(int16_t *)(player + 0x2) != -1) {
        game_engine_compute_look_angles_from_vector(&facing, *(int16_t *)(player + 0x2));
    }
    {
        datum_index effect = *(datum_index *)(*(uint8_t **)(global_globals_bytes + 0x174) + 0xc4);

        if (effect != k_datum_index_none) {
            game_engine_build_visible_cluster_bitmask((uint32_t *)(local_player_globals_bytes + 0x18), 0);
            effect_new_on_object(unit_index, effect, unit_index, -1, 0.0f, 0.0f, 0, 0);
        }
    }
    return placed;
}

#if 0
Original Ghidra decompilation (0x4757b0), from tools/pack.py 0x4757b0:

char FUN_004757b0(uint param_1,short *param_2)

{
  undefined *puVar1;
  char cVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  int iVar8;
  short sVar9;
  float local_7c [10];
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  int local_18;
  int local_14;
  uint local_10;
  float local_c;
  uint *local_8;

  iVar5 = DAT_008603b0;
  local_18 = (param_1 & 0xffff) * 0x200;
  local_10 = *(uint *)(local_18 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  local_18 = local_18 + *(int *)(DAT_0087a480 + 0x34);
  local_8 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_10 & 0xffff) * 0xc);
  cVar2 = '\0';
  if ((param_2 == (short *)0xffffffff) ||
     (uVar4 = object_get_root_object_index(), (short *)uVar4 == param_2)) {
    cVar2 = FUN_0055a500(local_10,param_2,0,0x40000000,0,0,1);
    iVar5 = DAT_008603b0;
    uVar4 = (uint)param_2;
  }
  else {
    uVar4 = object_get_root_object_index();
    local_14 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
    local_28 = *(float *)(local_14 + 0x68);
    local_24 = *(float *)(local_14 + 0x6c);
    if (local_28 * local_28 + local_24 * local_24 <= 0.0) {
      if (0.70710677 <= *(float *)(local_14 + 0x7c)) {
        local_28 = *(float *)(local_14 + 0x80);
        local_24 = *(float *)(local_14 + 0x84);
      }
      else {
        local_28 = *(float *)(local_14 + 0x74);
        local_24 = *(float *)(local_14 + 0x78);
      }
    }
    local_c = *(float *)(*(int *)((*local_8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x42c);
    local_20 = 0.0;
    local_1c = local_c * 3.0 + *(float *)(local_14 + 0xac);
    local_28 = -local_28;
    local_24 = -local_24;
    vector3d_normalize_with_length();
    matrix4x3_from_forward_up(local_7c);
    local_54 = *(undefined4 *)(local_14 + 0xa0);
    local_50 = *(undefined4 *)(local_14 + 0xa4);
    local_4c = *(undefined4 *)(local_14 + 0xa8);
    local_7c[0] = local_1c;
    local_14 = 0;
    do {
      if (cVar2 != '\0') break;
      matrix4x3_transform_point(local_7c);
      cVar2 = FUN_0055a500(local_10,uVar4,0,0x40000000,0,0,1);
      if (cVar2 == '\0') {
        sVar9 = 0;
        do {
          if (cVar2 != '\0') break;
          random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
          iVar5 = (int)(short)((random_seed_global >> 0x10) * (int)DAT_006b7af8 >> 0x10);
          local_28 = *(float *)(DAT_006b7af4 + iVar5 * 0xc);
          iVar5 = DAT_006b7af4 + iVar5 * 0xc;
          local_24 = *(float *)(iVar5 + 4);
          local_20 = *(float *)(iVar5 + 8);
          local_40 = local_28 * local_c + local_34;
          local_3c = local_24 * local_c + local_30;
          local_38 = local_20 * local_c + local_2c;
          cVar2 = FUN_0055a500(local_10,uVar4,0,0x40000000,0,0,1);
          sVar9 = sVar9 + 1;
        } while (sVar9 < 8);
      }
      local_14 = local_14 + 1;
      iVar5 = DAT_008603b0;
    } while ((ushort)local_14 < 9);
  }
  *(undefined2 *)(local_18 + 0x3c) = 0xffff;
  if (cVar2 == '\0') {
LAB_00475bb0:
    FUN_004760b0(uVar4);
  }
  else {
    iVar8 = 0;
    sVar9 = 0;
    if (0 < *(int *)(global_scenario + 0x39c)) {
      do {
        if (((*(short *)(*(int *)(global_scenario + 0x3a0) + iVar8 * 8 + 2) == DAT_0069e8d8) &&
            (*(int *)(local_18 + 0x34) != -1)) &&
           (cVar3 = scenario_trigger_volume_contains_point(), cVar3 != '\0')) {
          cVar2 = '\0';
          goto LAB_00475bb0;
        }
        sVar9 = sVar9 + 1;
        iVar8 = (int)sVar9;
      } while (iVar8 < *(int *)(global_scenario + 0x39c));
    }
    puVar1 = PTR_DAT_00696714;
    local_8[0x1a] = *(uint *)PTR_DAT_00696714;
    local_8[0x1b] = *(uint *)(puVar1 + 4);
    local_8[0x1c] = *(uint *)(puVar1 + 8);
    if (uVar4 != 0xffffffff) {
      iVar8 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
      local_28 = *(float *)(iVar8 + 0x74);
      local_24 = *(float *)(iVar8 + 0x78);
      local_20 = *(float *)(iVar8 + 0x7c);
      sVar9 = (short)uVar4;
      param_2 = (short *)0x0;
      if ((-1 < sVar9) && (sVar9 < *(short *)(iVar5 + 0x20))) {
        psVar6 = (short *)((int)*(short *)(iVar5 + 0x22) * (int)sVar9 + *(int *)(iVar5 + 0x34));
        sVar9 = *psVar6;
        if ((sVar9 != 0) && ((sVar7 = (short)(uVar4 >> 0x10), sVar7 == 0 || (sVar9 == sVar7)))) {
          param_2 = psVar6;
        }
      }
      if ((((param_2 != (short *)0x0) && ((1 << (*(byte *)((int)param_2 + 3) & 0x1f) & 1U) != 0)) &&
          (iVar5 = *(int *)(param_2 + 4), iVar5 != 0)) && (*(uint *)(iVar5 + 0x4d4) != 0xffffffff))
      {
        local_8[0x135] = *(uint *)(iVar5 + 0x4d4);
        *(undefined1 *)((int)local_8 + 0x4d3) = *(undefined1 *)(iVar5 + 0x4d3);
      }
      local_8[0x89] = (uint)local_28;
      local_8[0x8a] = (uint)local_24;
      local_8[0x8b] = (uint)local_20;
      local_8[0x95] = (uint)local_28;
      local_8[0x8c] = (uint)local_28;
      local_8[0x96] = (uint)local_24;
      local_8[0x8d] = (uint)local_24;
      local_8[0x97] = (uint)local_20;
      local_8[0x8e] = (uint)local_20;
      if (*(short *)(local_18 + 2) != -1) {
        game_engine_compute_look_angles_from_vector();
      }
      if (*(int *)(*(int *)(DAT_00746fa0 + 0x174) + 0xc4) != -1) {
        FUN_004782a0(DAT_0087a478 + 0x18,0);
        FUN_004507a0(local_10,0xffffffff,0,0,0,0);
        return cVar2;
      }
    }
  }
  return cVar2;
}
#endif
