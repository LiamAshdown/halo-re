// hs_object_detach_and_place_at_location  (Ghidra: FUN_00487f50)
// address 0x487f50, size 1568 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x487f50..0x48856f (the draft called most helpers without their arguments; object_teleport
//   crashed in object_set_position_and_orientation). Places an object on a cutscene flag (scenario +0x4e8, 0x5c
//   bytes: position +0x24, yaw +0x30, pitch +0x34). AX = the flag, stack: object, teleport, face.
//   - teleport with a parent: a unit (object_try_and_get mask 3) is taken out of its seat -- unless this is a network
//     client -- keeping it where the seat marker put it: the seat (parent tag +0x2e8, 0x11c each; +0x24 marker)
//     marker offset from the unit's root node (+0x1f2 node matrices) and the model's root node (unit tag +0x34 model,
//     +0xbc nodes; translation +0x28, matrix +0x68) give the new position and basis; a driver leaving puts the
//     vehicle in state 0x25; seat bookkeeping (+0x32c/+0x330, +0x324/+0x328 both ways, +0x2f0, +0x2a7 = 2), light
//     attachments, seat occupants, weapon, the exit animation request {0x14, 0}, the biped ground fix-up, bounds, the
//     vehicle's empty time (+0x5ac), a client's cleared player input and the scripted event 9; a non-unit just snaps
//     off its parent marker;
//   - then the object is woken, and a unit faces the flag (desired facing/aiming/looking +0x224/+0x230/+0x254 when
//     `face`; its seat-relative facing when parented) and, for a player's unit, the player is moved (0x475c60) and
//     its look angles set (0x470d80); objects without a player take the flag's position/forward directly.
// blam-cc: AX -> location_index, stack -> object_index, teleport, face

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "fn_hs.h"
#include "fn_game.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern Scenario *global_scenario;
extern data_array *player_data;      // 0x0087a480
extern int16_t network_game_mode; // 0x00719720 (1 = client)
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t *network_client; // 0x0071c2d8, +0xf48 the update history

extern double cos(double x);
extern double sin(double x);

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up,
    real_point3d *position); // 0x4f51c0, stack, EDI position
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum_markers); // 0x4f6080
extern void object_reset_velocity_and_wake(uint32_t object_index); // 0x4f5160
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0
extern uint8_t player_attach_unit_to_parent(uint32_t player_index, uint32_t target_object, void *local_offset); // 0x475c60
extern void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index); // 0x55add0, stack, EDI
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern void player_update_history_free_all(void *history); // 0x4e6f20
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI

extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, EAX, ECX
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0, EAX, EDX, stack
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

static const int8_t k_unit_exit_seat_request[2] = {0x14, 0};

#define OBJ(i) ((uint8_t *)((object_header *)object_data->data)[(i) & 0xffff].data)

// 0x487f96..0x4883c4: take a unit out of its seat, keeping its world placement
static void hs_unit_leave_seat(uint32_t object_index)
{
    uint8_t *unit = OBJ(object_index);
    datum_index parent_index = ((unit_object *)unit)->base.parent_object;

    if (parent_index != k_datum_index_none && ((unit_object *)unit)->unit.vehicle_seat_index != -1) {
        uint8_t *parent = OBJ(parent_index);
        uint8_t *parent_tag = (uint8_t *)tag_instances[*(datum_index *)parent & 0xffff].data;
        uint8_t *seat = *(uint8_t **)(parent_tag + 0x2e8) + ((unit_object *)unit)->unit.vehicle_seat_index * 0x11c;
        real_matrix4x3 *nodes = (real_matrix4x3 *)(unit + ((unit_object *)unit)->base.nodes.offset);
        uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
        uint8_t *model = (uint8_t *)tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id & 0xffff].data;
        uint8_t *root_node = *(uint8_t **)(model + 0xbc);
        real_vector3d root_offset = *(real_vector3d *)(root_node + 0x28);
        real_matrix4x3 *root_matrix = (real_matrix4x3 *)(root_node + 0x68);
        object_marker marker;
        real_vector3d delta;
        real_point3d position;
        real_matrix4x3 basis;

        object_get_node_local_transform(parent_index, (char *)(seat + 0x24), &marker, 1);
        delta.i = nodes->position.x - marker.node_transform.position.x;
        delta.j = nodes->position.y - marker.node_transform.position.y;
        delta.k = nodes->position.z - marker.node_transform.position.z;
        if (*(datum_index *)(parent + 0x324) == object_index && (int8_t)parent[0x2a3] != 0x25 &&
            ((unit_object *)unit)->base.parent_object != k_datum_index_none) {
            unit_try_set_animation_state(((unit_object *)unit)->base.parent_object, 0x25);
        }
        ((unit_object *)unit)->unit.last_parent_object_index = parent_index;
        ((unit_object *)unit)->unit.last_seat_change_tick = game_time->game_time;
        if (((unit_object *)unit)->unit.driver_unit_index == object_index) {
            ((unit_object *)unit)->unit.driver_unit_index = k_datum_index_none;
        }
        if (((unit_object *)unit)->unit.gunner_unit_index == object_index) {
            ((unit_object *)unit)->unit.gunner_unit_index = k_datum_index_none;
        }
        object_snap_to_parent_marker_and_detach(object_index);
        position.x = delta.i + ((unit_object *)unit)->base.position.x;
        position.y = delta.j + ((unit_object *)unit)->base.position.y;
        position.z = delta.k + ((unit_object *)unit)->base.position.z - root_offset.k;
        object_set_position_and_orientation(object_index, 0, 0, &position);

        unit = OBJ(object_index);
        matrix4x3_multiply_procedure((real_matrix4x3 *)(unit + ((unit_object *)unit)->base.nodes.offset), root_matrix, &basis);
        *(real_vector3d *)&((unit_object *)unit)->base.forward.i = basis.forward;
        *(real_vector3d *)&((unit_object *)unit)->base.up.i = basis.up;

        unit = OBJ(object_index);
        unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
        if (*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id != k_datum_index_none) {
            if ((((unit_object *)unit)->base.flags & 1) != 0) {
                object_for_each_light_attachment(object_index, 0, 1);
            }
            if (*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id != k_datum_index_none) {
                ((unit_object *)unit)->base.flags &= ~1u;
                ((uint8_t *)&((object_header *)object_data->data)[object_index & 0xffff])[2] |= 2;
            }
        }
        ((unit_object *)unit)->unit.vehicle_seat_index = -1;
        unit[0x2a7] = 2;
        if (*(datum_index *)(parent + 0x324) == object_index) {
            *(datum_index *)(parent + 0x324) = k_datum_index_none;
        }
        if (*(datum_index *)(parent + 0x328) == object_index) {
            *(datum_index *)(parent + 0x328) = k_datum_index_none;
        }
        unit_recompute_seat_occupants(parent_index);
        unit_pick_and_ready_next_weapon(object_index);
        unit_update_animation_state_machine(object_index, k_unit_exit_seat_request);
        unit = OBJ(object_index);
        *(real_vector3d *)(unit + ((unit_object *)unit)->base.node_function_values.offset + 0x10) = root_offset;
        if (((unit_object *)unit)->base.type == 0) {
            unit_reset_orientation_and_find_position(object_index, parent_index); // EDI = the seat parent
        }
        object_recalculate_bounding_radius_recursive(object_index);
        if (unit_all_seats_unoccupied(parent_index) == 1) {
            uint8_t *vehicle = (uint8_t *)object_try_and_get(parent_index, 2);

            if (vehicle != 0) {
                ((vehicle_object *)vehicle)->vehicle.network_update_tick = game_time->game_time;
            }
        }
        unit = OBJ(object_index);
        if (network_game_mode == 1) {
            uint8_t *player = (uint8_t *)datum_get(((unit_object *)unit)->unit.controlling_player, player_data);

            if (player != 0 && ((struct player *)player)->local_player_index == -1) {
                *(uint32_t *)&((struct player *)player)->position_updates.read_index = 0;
                *(uint32_t *)&((struct player *)player)->position_updates.write_index = 0;
                *(uint32_t *)&((struct player *)player)->vehicle_updates.read_index = 0;
                *(uint32_t *)&((struct player *)player)->vehicle_updates.write_index = 0;
            }
        }
    }

    // 0x488345
    {
        uint8_t *unit = OBJ(object_index);

        if (((unit_object *)unit)->base.network_role == 0) {
            unit_dispatch_scripted_event_9(1, (int32_t)object_index);
            unit = OBJ(object_index);
        }
        if (network_game_mode == 1) {
            datum_index player_index = ((unit_object *)unit)->unit.controlling_player;
            int16_t index = (int16_t)player_index;
            int16_t salt = (int16_t)(player_index >> 16);

            if (player_index != k_datum_index_none && index >= 0 && index < player_data->maximum_count) {
                uint8_t *player = (uint8_t *)player_data->data + index * player_data->size;
                int16_t identifier = *(int16_t *)player;

                if (identifier != 0 && (salt == 0 || identifier == salt) && ((struct player *)player)->local_player_index != -1 &&
                    network_client != 0) {
                    player_update_history_free_all(*(void **)(network_client + 0xf48));
                }
            }
        }
    }
}

void hs_object_detach_and_place_at_location(int16_t location_index, datum_index object_index,
    char detach_from_parent, char reorient)
{
    uint8_t *flag;
    uint8_t *placed;
    uint8_t *player = 0;
    real_vector3d forward;
    real_vector3d local_forward;
    object *unit;

    if (object_index == k_datum_index_none) {
        return;
    }
    flag = (uint8_t *)global_scenario->cutscene_flags.pointer + location_index * 0x5c;
    placed = OBJ(object_index);

    if (detach_from_parent && *(datum_index *)(placed + 0x11c) != k_datum_index_none) {
        if (object_try_and_get(object_index, 3) == 0) {
            object_snap_to_parent_marker_and_detach(object_index);
        } else if (network_game_mode != 1) {
            hs_unit_leave_seat(object_index);
        }
    }

    // 0x4883d4
    forward.i = (float)(cos((double)*(float *)(flag + 0x30)) * cos((double)*(float *)(flag + 0x34)));
    forward.j = (float)(sin((double)*(float *)(flag + 0x30)) * cos((double)*(float *)(flag + 0x34)));
    forward.k = (float)sin((double)*(float *)(flag + 0x34));
    object_reset_velocity_and_wake(object_index);

    unit = object_try_and_get(object_index, 3);
    if (unit != 0) {
        uint8_t *unit_bytes = (uint8_t *)unit;
        datum_index player_index = player_index_from_unit_index(object_index);

        if (*(datum_index *)(unit_bytes + 0x11c) != k_datum_index_none) {
            uint8_t *parent = OBJ(*(datum_index *)(unit_bytes + 0x11c));
            real_matrix4x3 *node = (real_matrix4x3 *)(parent + *(int16_t *)(parent + 0x1f2) +
                (int8_t)unit_bytes[0x120] * 0x34);
            real_matrix4x3 inverse;

            matrix4x3_inverse(&inverse, node);
            matrix4x3_transform_normal(&local_forward, &forward, &inverse);
        } else {
            local_forward = forward;
        }
        if (reorient) {
            *(real_vector3d *)(unit_bytes + 0x224) = forward;
            *(real_vector3d *)(unit_bytes + 0x230) = forward;
            *(real_vector3d *)(unit_bytes + 0x254) = forward;
        }
        if (player_index != k_datum_index_none) {
            player = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
            if (detach_from_parent) {
                player_attach_unit_to_parent(player_index, 0xffffffff, flag + 0x24);
            }
            if (reorient && ((struct player *)player)->local_player_index != -1) {
                game_engine_compute_look_angles_from_vector(&local_forward, ((struct player *)player)->local_player_index);
            }
        }
    }

    // 0x488529
    object_set_position_and_orientation(object_index,
        (reorient && player == 0) ? &forward : 0, 0,
        (detach_from_parent && player == 0) ? (real_point3d *)(flag + 0x24) : 0);
}

#if 0
Original Ghidra decompilation (0x487f50):

void FUN_00487f50(uint param_1,char param_2,char param_3)

{
  byte *pbVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  float fVar5;
  float fVar6;
  char cVar7;
  short in_AX;
  int iVar8;
  int iVar9;
  uint uVar10;
  float *pfVar11;
  short sVar12;
  short sVar13;
  int iVar14;
  int iVar15;
  float10 fVar16;
  float10 fVar17;
  float local_dc;
  float local_d8;
  float local_d4;
  uint local_d0;
  int local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [96];
  float local_18;
  float local_14;
  float local_10;
  iVar15 = DAT_008603b0;
  if (param_1 == 0xffffffff) {
    return;
  }
  iVar8 = in_AX * 0x5c + *(int *)(DAT_00746f8c + 0x4e8);
  local_cc = 0;
  if (param_2 != '\0') {
    iVar14 = (param_1 & 0xffff) * 0xc;
    puVar2 = *(uint **)(iVar14 + 8 + *(int *)(DAT_008603b0 + 0x34));
    if (puVar2[0x47] != 0xffffffff) {
      iVar9 = object_try_and_get(3);
      if (iVar9 == 0) {
        FUN_004f6610(param_1);
      }
      else if (DAT_00719720 != 1) {
        local_d0 = puVar2[0x47];
        iVar9 = DAT_0087a480;
        if ((local_d0 != 0xffffffff) && ((short)puVar2[0xbc] != -1)) {
          puVar3 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + (local_d0 & 0xffff) * 0xc);
          iVar15 = *(int *)(*(int *)(iVar15 + 0x34) + 8 + iVar14);
          iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
          FUN_004f6080(local_d0,*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                        0x2e8) + 0x24 + (short)puVar2[0xbc] * 0x11c,local_78,1);
          local_c8 = *(float *)(iVar15 + 0x28) - local_18;
          local_c4 = *(float *)(iVar15 + 0x2c) - local_14;
          iVar9 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 +
                                                       DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                   + DAT_0087bc14) + 0xbc);
          local_c0 = *(float *)(iVar15 + 0x30) - local_10;
          local_dc = *(float *)(iVar9 + 0x28);
          local_d8 = *(float *)(iVar9 + 0x2c);
          local_d4 = *(float *)(iVar9 + 0x30);
          if ((puVar3[0xc9] == param_1) &&
             ((*(char *)((int)puVar3 + 0x2a3) != '%' && (puVar2[0x47] != 0xffffffff)))) {
            unit_try_set_animation_state(puVar2[0x47],0x25);
          }
          iVar15 = DAT_006f1d6c;
          puVar2[0xcb] = local_d0;
          puVar2[0xcc] = *(uint *)(iVar15 + 0xc);
          if (puVar2[0xc9] == param_1) {
            puVar2[0xc9] = 0xffffffff;
          }
          if (puVar2[0xca] == param_1) {
            puVar2[0xca] = 0xffffffff;
          }
          FUN_004f6610(param_1);
          local_bc = local_c8 + (float)puVar2[0x17];
          local_b8 = local_c4 + (float)puVar2[0x18];
          local_b4 = (local_c0 + (float)puVar2[0x19]) - local_d4;
          FUN_004f51c0(param_1,0,0);
          iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
          (*(code *)PTR_matrix4x3_multiply_00696664)
                    (*(short *)(iVar15 + 0x1f2) + iVar15,iVar9 + 0x68,local_b0);
          puVar2[0x1d] = uStack_ac;
          puVar2[0x1e] = uStack_a8;
          puVar2[0x1f] = uStack_a4;
          puVar2[0x20] = uStack_94;
          puVar2[0x21] = uStack_90;
          iVar15 = DAT_008603b0;
          puVar2[0x22] = uStack_8c;
          puVar4 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar14);
          iVar15 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar15 + 0x34) != -1) {
            if ((puVar4[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar15 + 0x34) != -1) {
              iVar15 = *(int *)(DAT_008603b0 + 0x34);
              puVar4[4] = puVar4[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar15 + iVar14 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
          *(undefined2 *)(puVar2 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar2 + 0x2a7) = 2;
          if (puVar3[0xc9] == param_1) {
            puVar3[0xc9] = 0xffffffff;
          }
          if (puVar3[0xca] == param_1) {
            puVar3[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          FUN_00565420(param_1);
          pfVar11 = (float *)(*(short *)((int)puVar2 + 0x1ea) + 0x10 + (int)puVar2);
          *pfVar11 = local_dc;
          pfVar11[1] = local_d8;
          pfVar11[2] = local_d4;
          if ((short)puVar2[0x2d] == 0) {
            FUN_0055add0(param_1);
          }
          object_recalculate_bounding_radius_recursive(param_1);
          cVar7 = FUN_00566910();
          if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
            *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar9 = DAT_0087a480;
          if (((DAT_00719720 == 1) && (iVar15 = datum_get(), iVar15 != 0)) &&
             (*(short *)(iVar15 + 2) == -1)) {
            *(undefined4 *)(iVar15 + 0x180) = 0;
            *(undefined4 *)(iVar15 + 0x17c) = 0;
            *(undefined4 *)(iVar15 + 0x1e0) = 0;
            *(undefined4 *)(iVar15 + 0x1dc) = 0;
          }
        }
        if (puVar2[1] == 0) {
          FUN_0056c370(1);
          iVar9 = DAT_0087a480;
        }
        if (((DAT_00719720 == 1) && (uVar10 = puVar2[0x86], uVar10 != 0xffffffff)) &&
           ((sVar13 = (short)uVar10, -1 < sVar13 && (sVar13 < *(short *)(iVar9 + 0x20))))) {
          iVar15 = (int)*(short *)(iVar9 + 0x22) * (int)sVar13;
          sVar13 = *(short *)(iVar15 + *(int *)(iVar9 + 0x34));
          if (((sVar13 != 0) &&
              ((sVar12 = (short)(uVar10 >> 0x10), sVar12 == 0 || (sVar13 == sVar12)))) &&
             ((*(short *)(iVar15 + *(int *)(iVar9 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0)))) {
            player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
          }
        }
      }
    }
  }
  fVar16 = (float10)fcos((float10)*(float *)(iVar8 + 0x34));
  fVar17 = (float10)fcos((float10)*(float *)(iVar8 + 0x30));
  local_dc = (float)(fVar17 * fVar16);
  fVar17 = (float10)fsin((float10)*(float *)(iVar8 + 0x30));
  local_d8 = (float)(fVar17 * fVar16);
  fVar16 = (float10)fsin((float10)*(float *)(iVar8 + 0x34));
  local_d4 = (float)fVar16;
  FUN_004f5160(param_1);
  iVar15 = object_try_and_get(3);
  if (iVar15 == 0) {
LAB_00488529:
    if ((param_3 != '\0') && (local_cc == 0)) {
      pfVar11 = &local_dc;
      goto LAB_00488540;
    }
  }
  else {
    uVar10 = FUN_00474db0(param_1);
    fVar6 = local_d8;
    fVar5 = local_dc;
    if (*(int *)(iVar15 + 0x11c) == -1) {
      local_c8 = local_dc;
      local_c4 = local_d8;
      local_c0 = local_d4;
    }
    else {
      matrix4x3_inverse();
      matrix4x3_transform_normal(local_b0);
    }
    if (param_3 != '\0') {
      *(float *)(iVar15 + 0x224) = fVar5;
      *(float *)(iVar15 + 0x228) = fVar6;
      *(float *)(iVar15 + 0x22c) = local_d4;
      *(float *)(iVar15 + 0x230) = fVar5;
      *(float *)(iVar15 + 0x254) = fVar5;
      *(float *)(iVar15 + 0x234) = fVar6;
      *(float *)(iVar15 + 600) = fVar6;
      *(float *)(iVar15 + 0x238) = local_d4;
      *(float *)(iVar15 + 0x25c) = local_d4;
    }
    if (uVar10 == 0xffffffff) goto LAB_00488529;
    local_cc = (uVar10 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
    if (param_2 != '\0') {
      FUN_00475c60(uVar10,0xffffffff,iVar8 + 0x24);
    }
    if (param_3 != '\0') {
      if (*(short *)(local_cc + 2) != -1) {
        game_engine_compute_look_angles_from_vector();
      }
      goto LAB_00488529;
    }
  }
  pfVar11 = (float *)0x0;
LAB_00488540:
  FUN_004f51c0(param_1,pfVar11,0);
  return;
}
#endif
