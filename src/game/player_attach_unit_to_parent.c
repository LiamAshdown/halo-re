// player_attach_unit_to_parent  (Ghidra: FUN_00475c60; named per this rewrite)
// address 0x475c60, size 1096 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x475c60..0x4760a7. Stack: (player, target object, point). When the player's biped rides
//   something (and this is not a network client), it is first taken out of its seat exactly like the seat-exit
//   inline elsewhere (marker-relative position, seat bookkeeping, exit animation, placement around the vehicle,
//   empty-vehicle stamp), then the scripted event 9 fires for a non-networked unit and a client drops its
//   prediction history; finally 0x4757b0 places the player with all three arguments. Returns its result (0 when
//   the player has no biped). The draft read the driver field as the parent and called the seat / weapon
//   helpers without operands.
// blam-cc: stack -> player_index, target_object, local_offset

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *player_data;      // 0x0087a480
extern int16_t network_game_mode; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 (via 0x696664)
extern void player_update_history_free_all(void *history); // 0x4e6f20
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up,
    real_point3d *position); // 0x4f51c0, stack, EDI position
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack
extern void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index); // 0x55add0, stack, EDI
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern uint8_t player_find_placement_position(uint32_t player_index, datum_index target_object,
    real_point3d *point); // 0x4757b0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define OBJECT_HEADER(h) (((object_header *)object_data->data)[(h) & 0xffff])
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// 0x475cf9..0x476002: the seat-exit inline (same as unit_detach_from_seat's biped_detach_from_seat).
static void player_unit_exit_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + ((struct object *)self)->nodes.offset;
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + *(int16_t *)(self + 0x2f0) * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)(TAG_DATA(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index && vehicle[0x2a3] != 0x25 &&
        ((struct object *)self)->parent_object != k_datum_index_none) {
        unit_try_set_animation_state(((struct object *)self)->parent_object, 0x25);
    }
    *(datum_index *)(self + 0x32c) = vehicle_index;
    *(int32_t *)(self + 0x330) = game_time->game_time;
    if (*(datum_index *)(self + 0x324) == object_index) {
        *(datum_index *)(self + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(self + 0x328) == object_index) {
        *(datum_index *)(self + 0x328) = k_datum_index_none;
    }
    object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((struct object *)self)->position.x;
    position.y = offset.y + ((struct object *)self)->position.y;
    position.z = offset.z + ((struct object *)self)->position.z - default_translation.z;
    object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((struct object *)self)->forward.i = basis.forward;
    *(real_vector3d *)&((struct object *)self)->up.i = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && (object[0x10] & 1) != 0) {
            object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    *(int16_t *)(self + 0x2f0) = -1;
    self[0x2a7] = 2;
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((vehicle_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    unit_recompute_seat_occupants(vehicle_index);
    unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + ((struct object *)self)->node_function_values.offset + 0x10) = default_translation;
    if (((struct object *)self)->type == 0) {
        unit_reset_orientation_and_find_position(object_index, vehicle_index);
    }
    object_recalculate_bounding_radius_recursive(object_index);
    if (unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (network_game_mode == 1) {
        uint8_t *player = (uint8_t *)datum_get(*(datum_index *)(self + 0x218), player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}

uint8_t player_attach_unit_to_parent(uint32_t player_index, uint32_t target_object, void *local_offset)
{
    uint32_t unit_index = *(datum_index *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200 + 0x34);
    uint8_t *biped = (uint8_t *)object_try_and_get(unit_index, 1);

    if (biped == 0) {
        return 0;
    }
    if (((biped_object *)biped)->base.parent_object != k_datum_index_none && network_game_mode != 1) {
        uint8_t *self = OBJECT_DATA(unit_index);
        datum_index parent = ((struct object *)self)->parent_object;

        if (parent != k_datum_index_none && *(int16_t *)(self + 0x2f0) != -1) {
            player_unit_exit_seat(unit_index, parent);
        }
        // 0x476005
        if (((struct object *)self)->network_role == 0) {
            unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
        }
        if (network_game_mode == 1) {
            datum_index player_handle = *(datum_index *)(self + 0x218);
            int16_t index = (int16_t)player_handle;
            int16_t salt = (int16_t)(player_handle >> 16);

            if (player_handle != k_datum_index_none && index >= 0 &&
                index < *(int16_t *)((uint8_t *)player_data + 0x20)) {
                uint8_t *player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;

                if (*(int16_t *)player != 0 && (salt == 0 || *(int16_t *)player == salt) &&
                    ((struct player *)player)->local_player_index != -1 && network_client != 0) {
                    player_update_history_free_all(*(void **)&network_client->update_history);
                }
            }
        }
    }
    return player_find_placement_position(player_index, target_object, (real_point3d *)local_offset);
}

#if 0
Original Ghidra decompilation (0x475c60), from tools/pack.py 0x475c60:

undefined4 FUN_00475c60(uint param_1,undefined4 param_2,undefined4 param_3)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint *puVar4;
  int iVar5;
  uint *puVar6;
  uint uVar7;
  char cVar8;
  int iVar9;
  undefined4 uVar10;
  short sVar11;
  short sVar12;
  undefined1 local_e4 [96];
  float local_84;
  float local_80;
  float local_7c;
  undefined1 local_74 [4];
  uint local_70;
  uint local_6c;
  uint local_68;
  uint local_58;
  uint local_54;
  uint local_50;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  undefined4 local_24;
  undefined4 local_20;
  float local_1c;
  uint *local_18;
  int local_14;
  uint local_10;
  uint local_c;
  undefined4 local_8;

  uVar3 = *(uint *)((param_1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  local_10 = uVar3;
  iVar9 = object_try_and_get(1);
  if (iVar9 == 0) {
    return 0;
  }
  if ((*(int *)(iVar9 + 0x11c) != -1) && (DAT_00719720 != 1)) {
    local_8 = (uVar3 & 0xffff) * 0xc;
    puVar4 = *(uint **)(local_8 + 8 + *(int *)(DAT_008603b0 + 0x34));
    local_c = puVar4[0x47];
    if ((local_c != 0xffffffff) && ((short)puVar4[0xbc] != -1)) {
      local_18 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc);
      iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
      iVar9 = *(short *)(iVar9 + 0x1f2) + iVar9;
      object_get_node_local_transform
                (local_c,*(int *)(*(int *)((*local_18 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                 0x2e8) + 0x24 + (short)puVar4[0xbc] * 0x11c,local_e4,1);
      local_30 = *(float *)(iVar9 + 0x28) - local_84;
      local_2c = *(float *)(iVar9 + 0x2c) - local_80;
      iVar5 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                          + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
      local_14 = iVar5 + 0x68;
      local_24 = *(undefined4 *)(iVar5 + 0x28);
      local_28 = *(float *)(iVar9 + 0x30) - local_7c;
      local_20 = *(undefined4 *)(iVar5 + 0x2c);
      local_1c = *(float *)(iVar5 + 0x30);
      if (((local_18[0xc9] == uVar3) && (*(char *)((int)local_18 + 0x2a3) != '%')) &&
         (puVar4[0x47] != 0xffffffff)) {
        unit_try_set_animation_state(puVar4[0x47],0x25);
      }
      iVar9 = DAT_006f1d6c;
      puVar4[0xcb] = local_c;
      puVar4[0xcc] = *(uint *)(iVar9 + 0xc);
      if (puVar4[0xc9] == uVar3) {
        puVar4[0xc9] = 0xffffffff;
      }
      if (puVar4[0xca] == uVar3) {
        puVar4[0xca] = 0xffffffff;
      }
      FUN_004f6610(uVar3);
      local_3c = local_30 + (float)puVar4[0x17];
      local_38 = local_2c + (float)puVar4[0x18];
      local_34 = (local_28 + (float)puVar4[0x19]) - local_1c;
      object_set_position_and_orientation(uVar3,0,0);
      uVar7 = local_8;
      iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(short *)(iVar9 + 0x1f2) + iVar9,local_14,local_74);
      puVar4[0x1d] = local_70;
      puVar4[0x1e] = local_6c;
      puVar4[0x1f] = local_68;
      puVar4[0x20] = local_58;
      puVar4[0x21] = local_54;
      iVar9 = DAT_008603b0;
      puVar4[0x22] = local_50;
      puVar6 = *(uint **)(*(int *)(iVar9 + 0x34) + 8 + uVar7);
      local_14 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(local_14 + 0x34) != -1) {
        if ((puVar6[4] & 1) != 0) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_14 + 0x34) != -1) {
          iVar9 = *(int *)(DAT_008603b0 + 0x34);
          puVar6[4] = puVar6[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar9 + local_8 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
      }
      *(undefined2 *)(puVar4 + 0xbc) = 0xffff;
      *(undefined1 *)((int)puVar4 + 0x2a7) = 2;
      if (local_18[0xc9] == uVar3) {
        local_18[0xc9] = 0xffffffff;
      }
      if (local_18[0xca] == uVar3) {
        local_18[0xca] = 0xffffffff;
      }
      FUN_0056ce30();
      FUN_0056d6a0();
      uVar3 = local_10;
      local_8 = (uint)CONCAT12(0x14,(undefined2)local_8);
      FUN_00565420(local_10);
      puVar2 = (undefined4 *)(*(short *)((int)puVar4 + 0x1ea) + 0x10 + (int)puVar4);
      *puVar2 = local_24;
      puVar2[1] = local_20;
      puVar2[2] = local_1c;
      if ((short)puVar4[0x2d] == 0) {
        FUN_0055add0(uVar3);
      }
      object_recalculate_bounding_radius_recursive(uVar3);
      cVar8 = FUN_00566910();
      if ((cVar8 == '\x01') && (iVar9 = object_try_and_get(2), iVar9 != 0)) {
        *(undefined4 *)(iVar9 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
      }
      if (((DAT_00719720 == 1) && (iVar9 = datum_get(), iVar9 != 0)) &&
         (*(short *)(iVar9 + 2) == -1)) {
        *(undefined4 *)(iVar9 + 0x180) = 0;
        *(undefined4 *)(iVar9 + 0x17c) = 0;
        *(undefined4 *)(iVar9 + 0x1e0) = 0;
        *(undefined4 *)(iVar9 + 0x1dc) = 0;
      }
    }
    if (puVar4[1] == 0) {
      FUN_0056c370(1);
    }
    if ((((DAT_00719720 == 1) && (uVar3 = puVar4[0x86], uVar3 != 0xffffffff)) &&
        (sVar12 = (short)uVar3, -1 < sVar12)) && (sVar12 < *(short *)(DAT_0087a480 + 0x20))) {
      iVar9 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar12;
      sVar12 = *(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34));
      if ((((sVar12 != 0) && ((sVar11 = (short)(uVar3 >> 0x10), sVar11 == 0 || (sVar12 == sVar11))))
          && (*(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0))
      {
        player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      }
    }
  }
  uVar10 = FUN_004757b0(param_1,param_2,param_3);
  return uVar10;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
