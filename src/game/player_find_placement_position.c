// player_find_placement_position  (Ghidra: FUN_004757b0; named per this rewrite)
// address 0x4757b0, size 1181 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Searches nearby candidate positions, including
//   randomized jitter, for a placement where a player's unit does not collide with the world,
//   used when attaching/respawning a unit"); types/objects.h object (velocity +0x68, forward
//   +0x74, up +0x80, parent_object +0x11c); types/units.h unit_data (desired_facing_vector
//   +0x224, desired_aiming_vector +0x230, desired_looking_vector +0x254, biped_data
//   last_ground_object_index +0x4d4 / unknown_4d3); types/game.h player (bsp_cluster +0x3c,
//   local_player_index +0x02); game_engine_compute_look_angles_from_vector.c (this module) for
//   the tail-call shape. objdump -d -M intel --start-address=0x4757b0 --stop-address=0x475c60
//   bin/halo.exe was read for the register conventions of object_get_root_object_index (ECX)
//   and FUN_0055a500 (EDX -> a position pointer, plus 7 stack arguments).
// register convention: stack -> player_index, target_object.
//
// UNSURE (pervasive): the collision-probe routine FUN_0055a500, the random offset table at
// 0x006b7af4 (indexed via the global PRNG and a bounds value at 0x006b7af8), the tag-data float
// at absolute offset 0x42c, and effect_new_on_object's real signature are not established anywhere else
// in this codebase (existing files declare several mutually-incompatible signatures for
// effect_new_on_object); all are transcribed as literally as possible with raw offsets.
// reconciled: R46 biped_data +0x4d4 last_ground_surface_index -> last_ground_object_index (an object datum); the raw +0x4d4/+0x4d3 copies now go through biped_data

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;        // 0x0087a480
extern data_array *object_data;        // 0x008603b0
extern tag_instance *tag_instances;    // 0x0087bc14
extern Scenario *global_scenario;      // 0x00746f8c
extern Globals *global_globals;        // 0x00746fa0
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern player_globals *local_player_globals; // 0x0087a478
extern real_vector3d global_origin3d;  // 0x0065c230
extern random_seed random_seed_global;    // 0x00719cd0
extern int16_t random_point_table_count; // 0x006b7af8 (read with movsx from a word)
extern real_point3d random_point_table[]; // 0x006b7af4 (a fixed array, not a pointer variable)

extern datum_index object_get_root_object_index(datum_index object_index); // 0x4f6fb0, blam-cc: ECX -> object_index
extern uint8_t FUN_0055a500(real_point3d *position, datum_index unit_handle, datum_index exclude_object,
                             int32_t p3, uint32_t flags, int32_t p5, int32_t p6, int32_t p7);
    // 0x55a500, units module, not in this batch; blam-cc: EDX -> position, stack -> the rest; UNSURE
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, vector in ECX
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0
extern char scenario_trigger_volume_contains_point(int32_t trigger_volume_index, datum_index unit_handle); // 0x53f020
extern void game_engine_compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index); // 0x470d80
extern void player_release_unit_and_reset(uint32_t player_index, int32_t previous_unit_override); // this batch, 0x4760b0
extern void game_engine_build_visible_cluster_bitmask(void *out_bitmask, uint32_t flag); // this module's next batch, 0x4782a0
extern void effect_new_on_object(uint32_t param1, datum_index unit_handle, uint32_t p2, uint32_t p3,
                          uint32_t p4, uint32_t p5); // 0x4507a0, UNSURE signature (inconsistent
                          // across this codebase); blam-cc: ECX -> param1, stack -> the rest

// UNSURE, best-effort: see header. Places a player's unit either exactly at target_object's
// current root position (when that is already the player's own unit's root) or by probing
// nearby offsets -- first straight ahead of the root object, then eight random jittered points
// -- for a collision-free spot (FUN_0055a500). On success, seeds the unit's velocity to zero,
// resets its bsp_cluster, checks a bsp-switch trigger volume covering the new position, and
// seeds its desired facing/aiming/looking vectors and ground-surface cache from the target when
// given; on failure, forwards to FUN_004760b0.
uint8_t player_find_placement_position(uint32_t player_index, datum_index target_object)
{
    player *plr;
    datum_index unit_handle;
    object *unit_obj;
    uint8_t placed;
    int32_t final_target = (int32_t)target_object;

    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit_handle = plr->unit;
    unit_obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
    placed = 0;

    if (target_object == (uint32_t)-1 || object_get_root_object_index(target_object) == target_object) {
        placed = FUN_0055a500(0, unit_handle, target_object, 0, 0x40000000, 0, 0, 1); // UNSURE: position arg
        final_target = (int32_t)target_object;
    } else {
        datum_index root = object_get_root_object_index(target_object);
        object *root_obj = ((object_header *)object_data->data)[root & 0xffff].data;
        real_vector3d away;
        real_vector3d forward_probe;
        real_matrix4x3 basis;
        real_point3d probe;
        int32_t attempt;
        // UNSURE: tag-data float at absolute offset 0x42c
        float scale = *(float *)((uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data + 0x42c);

        away.i = root_obj->velocity.i;
        away.j = root_obj->velocity.j;
        if (away.i * away.i + away.j * away.j <= 0.0f) {
            if (root_obj->up.k >= 0.70710677f) {
                away.i = root_obj->up.i;
                away.j = root_obj->up.j;
            } else {
                away.i = root_obj->forward.i;
                away.j = root_obj->forward.j;
            }
        }
        away.i = -away.i;
        away.j = -away.j;
        away.k = 0.0f;
        vector3d_normalize_with_length(&away);
        matrix4x3_from_forward_up(&global_origin3d, &away, &basis); // UNSURE: up argument
        basis.forward.i = scale * 3.0f + root_obj->velocity.k; // UNSURE mapping of local_7c[0]

        for (attempt = 0; attempt < 9 && placed == 0; attempt = attempt + 1) {
            matrix4x3_transform_point(&probe, (real_point3d *)&basis, &basis);
            placed = FUN_0055a500(&probe, unit_handle, target_object, 0, 0x40000000, 0, 0, 1);
            if (placed == 0) {
                int16_t retry;
                for (retry = 0; retry < 8 && placed == 0; retry = retry + 1) {
                    int32_t index;
                    real_vector3d *rand_vec;

                    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                    index = (int16_t)(((int32_t)(random_seed_global >> 16) * (int32_t)random_point_table_count) >> 16);
                    rand_vec = (real_vector3d *)(random_point_table + index * 0xc);
                    probe.x = rand_vec->i * scale + basis.up.i; // UNSURE exact field mapping
                    probe.y = rand_vec->j * scale + basis.up.j;
                    probe.z = rand_vec->k * scale + basis.up.k;
                    placed = FUN_0055a500(&probe, unit_handle, target_object, 0, 0x40000000, 0, 0, 1);
                }
            }
        }
        final_target = (int32_t)target_object;
    }

    plr->bsp_cluster = -1;

    if (placed == 0) {
        player_release_unit_and_reset(player_index, final_target);
    } else {
        int32_t i;
        int32_t count = global_scenario->bsp_switch_trigger_volumes.count;
        ScenarioBSPSwitchTriggerVolume *volumes =
            (ScenarioBSPSwitchTriggerVolume *)global_scenario->bsp_switch_trigger_volumes.pointer;
        uint8_t found_trigger = 0;

        for (i = 0; i < count; i = i + 1) {
            if (volumes[i].source == (uint16_t)global_structure_bsp_index && plr->unit != (datum_index)-1 &&
                scenario_trigger_volume_contains_point(volumes[i].trigger_volume, plr->unit) != 0) {
                placed = 0;
                found_trigger = 1;
                break;
            }
        }

        if (!found_trigger) {
            unit_obj->velocity = global_origin3d;

            if (final_target != -1) {
                object *target_obj = ((object_header *)object_data->data)[final_target & 0xffff].data;
                unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                real_vector3d facing = target_obj->forward;

                int16_t index = (int16_t)final_target;
                object_header *target_header = 0;
                if (index >= 0 && index < object_data->maximum_count) {
                    object_header *candidate = &((object_header *)object_data->data)[index];
                    int16_t salt = (int16_t)((uint32_t)final_target >> 16);
                    if (candidate->identifier != 0 && (salt == 0 || candidate->identifier == salt)) {
                        target_header = candidate;
                    }
                }
                if (target_header != 0 && (1u << (target_header->type & 0x1f) & _object_mask_biped) != 0 &&
                    target_header->data != 0) {
                    object *biped_obj = target_header->data;
                    biped_data *source = (biped_data *)((uint8_t *)biped_obj + k_unit_object_size);
                    biped_data *dest = (biped_data *)((uint8_t *)unit_obj + k_unit_object_size);
                    if (source->last_ground_object_index != k_datum_index_none) {
                        dest->last_ground_object_index = source->last_ground_object_index;
                        dest->unknown_4d3 = source->unknown_4d3;
                    }
                }

                unit->desired_facing_vector = facing;
                unit->desired_aiming_vector = facing;
                unit->desired_looking_vector = facing;

                if (plr->local_player_index != -1) {
                    game_engine_compute_look_angles_from_vector(&facing, plr->local_player_index);
                }

                if (global_globals->player_information.pointer != 0 &&
                    *(int32_t *)((uint8_t *)global_globals->player_information.pointer + 0xc4) != -1) {
                    game_engine_build_visible_cluster_bitmask((uint8_t *)local_player_globals + 0x18, 0);
                    effect_new_on_object(0, unit_handle, 0, 0, 0, 0xffffffff); // UNSURE arg order
                    return placed;
                }
            }
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
