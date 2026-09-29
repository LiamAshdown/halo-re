// player_update_history_play  (Ghidra: player_update_history_play, already named)
// address 0x4e6ff0, size 1846 bytes
// name confidence: 0.9   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md; types/networking.h player_update_history /
// player_update_history_node / local_player_vehicle_update_ack; types/objects.h object_header /
// object; types/units.h unit_data / biped_data / vehicle_data; types/game.h vehicle_update_body;
// src/game/player_unit_has_parent.c (this batch's sibling call to the same unit_seat_flag_bit2, which
// fixes that callee's real signature); player_update_history_add.c (this batch, 0x4e6b50) for the
// node<->object field mapping, which this function performs in reverse (writing the snapshot back
// onto the live objects instead of capturing it).
// register convention: the officially recognized parameters are __cdecl stack arguments
// (history, unit_index, server_x, server_y, server_z, vehicle_ack); in addition, the call this
// function itself makes to player_update_history_find_and_prune passes on two register-only
// arguments (in_ECX, in_AL) that Ghidra never resolves to a caller-visible value anywhere in this
// batch -- see the UNSURE note below.
//   // blam-cc: EAX(AL) -> prune, ECX -> prune_target_id, stack -> history, unit_index, server_x,
//   //          server_y, server_z, vehicle_ack
// UNSURE (load-bearing): prune / prune_target_id are read straight out of undeclared registers in
// the original (`in_ECX`, `in_AL`) at every call site of this function found in this batch, so
// their real values are not recoverable without a full caller-side disassembly trace. They are
// modeled here as ordinary parameters per the "blam-cc" convention, forwarded unchanged to
// player_update_history_find_and_prune exactly as the decompile shows.
// UNSURE: vehicle_ack's type is inferred from its field offsets (+0x14 velocity, +0x20
// angular_velocity, +0x2c forward, +0x38 up) matching types/networking.h
// local_player_vehicle_update_ack::vehicle exactly; the decompile itself only shows it as an
// untyped pointer/flag (`param_6`).
// UNSURE: player_compute_view_forward_vector, unit_apply_control_block, biped_update, unit_propagate_position_delta_to_children, biped_update and object_update
// are called with zero visible arguments in the batch decompile inside the replay loop; Ghidra
// dropped whatever registers fed them (almost certainly the live unit_obj/vehicle_obj pointer),
// and that could not be re-derived without disassembly. They are declared and called exactly as
// shown (no arguments), which preserves the visible control flow without inventing values Ghidra
// itself could not recover.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

extern void *memcpy(void *dest, const void *src, int32_t count);
extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern data_array *object_data; // 0x008603b0
extern uint8_t unit_updates_suppressed; // 0x0071c419, UNSURE: set 1 for the duration of every
    // single object_update/biped_update call in the replay loop below

extern player_update_history_node *player_update_history_find_and_prune(
    player_update_history *history, int32_t target_id, uint8_t prune); // this module, 0x4e6f60
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0
extern uint8_t unit_seat_flag_bit2(datum_index parent_object, int16_t vehicle_seat_index); // 0x56cd10,
    // units module, not in this batch; blam-cc: EAX -> parent_object, CX -> vehicle_seat_index
extern void player_compute_view_forward_vector(void); // 0x473d70, game module, not in this batch; UNSURE: arguments
extern void unit_apply_control_block(void); // 0x5639f0, units module, not in this batch; UNSURE: arguments
extern void unit_propagate_position_delta_to_children(void); // 0x570cb0, units module, not in this batch; UNSURE: arguments
extern void biped_update(void);   // 0x5590a0, units module, not in this batch; UNSURE: arguments
extern void object_update(void); // 0x4f7ef0, objects module, not in this batch; UNSURE: arguments

// Replays every history node from the one player_update_history_find_and_prune leaves (after
// discarding everything up to and including prune_target_id, when prune is set) through the end
// of the list, restoring each node's saved unit (and, if applicable, vehicle) state and then
// re-running that many ticks of biped_update/object_update on top of it -- reconciling
// client-side prediction with a server-acknowledged starting position. Updates history's running
// call-count, tick totals and error-distance statistics (unknown_0c[0..7]) before returning.
int32_t player_update_history_play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack)
    // blam-cc: EAX(AL) -> prune, ECX -> prune_target_id, stack -> history, unit_index, server_x,
    //          server_y, server_z, vehicle_ack
{
    player_update_history_node *node;
    object *unit_obj;
    object *vehicle_obj;
    unit_data *unit_ext;
    biped_data *biped_ext;
    unit_data *vehicle_ext;
    datum_index parent_object;
    uint8_t in_vehicle_check;
    int32_t updates_this_call;
    int32_t ticks_this_call;
    int32_t remaining_ticks;
    int32_t result;
    real original_x, original_y, original_z;
    real server_start_x, server_start_y, server_start_z;
    real client_start_x, client_start_y, client_start_z;
    real end_x, end_y, end_z;
    real end2_x, end2_y, end2_z;
    real dx, dz;
    real distance;

    vehicle_obj = 0;
    node = player_update_history_find_and_prune(history, prune_target_id, prune);
    history->unknown_0c[0] = history->unknown_0c[0] + 1; // call count
    history->unknown_0c[3] = 0; // last-call update count
    history->unknown_0c[4] = 0; // last-call tick count

    if (unit_index == (datum_index)-1) {
        result = 0;
        player_update_history_log_write(1, 0, "Ignoring update [%d] due to unit_index == NONE");
        if (node != 0) {
            return result;
        }
    } else if (node != 0) {
        unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        unit_ext = (unit_data *)((uint8_t *)unit_obj + 0x1f4);
        biped_ext = (biped_data *)((uint8_t *)unit_obj + 0x4cc);
        parent_object = unit_obj->parent_object;

        if (parent_object != (datum_index)-1) {
            in_vehicle_check = unit_seat_flag_bit2(parent_object, unit_ext->vehicle_seat_index);
            if (in_vehicle_check != 1) {
                // Ghidra tests only AL (`(char)iVar8 != 1`) and returns the whole EAX; the
                // callee is declared to return uint8_t across the tree (see
                // src/game/player_unit_has_parent.c), which makes the two readings identical.
                return in_vehicle_check;
            }
            if (vehicle_ack == 0) {
                return 0;
            }
            if (node->has_vehicle != 1) {
                return (int32_t)vehicle_ack;
            }
            vehicle_obj = ((object_header *)object_data->data)[parent_object & 0xffff].data;
            vehicle_ext = (unit_data *)((uint8_t *)vehicle_obj + 0x1f4);
            if (((vehicle_data *)((uint8_t *)vehicle_obj + 0x4cc))->unknown_524 != 0) {
                ((vehicle_data *)((uint8_t *)vehicle_obj + 0x4cc))->unknown_524 = 0;
                return (int32_t)vehicle_obj;
            }
        }
        if (unit_ext->animation_state == _unit_animation_state_seat_exit ||
            unit_ext->animation_state == _unit_animation_state_seat_enter) {
            return (int32_t)vehicle_obj;
        }

        if (vehicle_obj == 0) {
            original_x = unit_obj->position.x;
            original_y = unit_obj->position.y;
            original_z = unit_obj->position.z;
            client_start_x = *(real *)(node->unit_state + 0x00);
            client_start_y = *(real *)(node->unit_state + 0x04);
            client_start_z = *(real *)(node->unit_state + 0x08);
        } else {
            original_x = vehicle_obj->position.x;
            original_y = vehicle_obj->position.y;
            original_z = vehicle_obj->position.z;
            client_start_x = *(real *)(node->vehicle_state + 0x00);
            client_start_y = *(real *)(node->vehicle_state + 0x04);
            client_start_z = *(real *)(node->vehicle_state + 0x08);
        }

        // Restore the unit's saved fields from the node, mirroring player_update_history_add's
        // capture of the same offsets in reverse.
        // REVIEW PASS 2026-09-20: the restore starts at the unit's *velocity* (object 0x68 <-
        // node+0x3c, Ghidra's piVar7[0xf]). The first draft also restored object 0x11c
        // (parent_object) and object 0x5c (position) from the node; neither write exists in
        // tools/pack.py 0x4e6ff0. Position is deliberately left alone here: the non-vehicle
        // branch below overwrites it with the server's position instead, and the vehicle branch
        // never touches it.
        unit_obj->velocity = *(real_vector3d *)(node->unit_state + 0x0c);
        unit_obj->forward = *(real_vector3d *)(node->unit_state + 0x18);
        unit_obj->animation_graph = *(datum_index *)(node->unit_state + 0x24);
        unit_obj->animation_index = *(int16_t *)(node->unit_state + 0x28);
        unit_obj->animation_frame = *(int16_t *)(node->unit_state + 0x2a);
        unit_obj->node_function_ticks_elapsed = *(int16_t *)(node->unit_state + 0x2c);
        unit_obj->node_function_count = *(int16_t *)(node->unit_state + 0x2e);
        memcpy(&unit_ext->animation_state_flags, node->unit_state + 0x30, 0x48);
        memcpy(&unit_ext->unknown_34c, node->unit_state + 0x78, 0x30);
        biped_ext->flags = *(uint32_t *)(node->unit_state + 0xa8);
        biped_ext->idle_trigger_counter = node->unit_state[0xac];
        biped_ext->flags_bit0_ticks = node->unit_state[0xad];
        biped_ext->flags_bit1_ticks = node->unit_state[0xae];
        biped_ext->target_lock_lost_ticks = node->unit_state[0xaf];
        biped_ext->unknown_508 = *(int16_t *)(node->unit_state + 0xb0);
        biped_ext->crouch_fraction = *(float *)(node->unit_state + 0xb4);
        biped_ext->ground_normal = *(real_vector3d *)(node->unit_state + 0xb8);
        biped_ext->ground_plane_offset = *(uint32_t *)(node->unit_state + 0xc4);
        biped_ext->frame_counter = node->unit_state[0xc8];
        biped_ext->frame_counter_limit = node->unit_state[0xc9];
        biped_ext->movement_state = node->unit_state[0xca];
        biped_ext->ground_surface_index = *(datum_index *)(node->unit_state + 0xcc);

        if (vehicle_obj == 0) {
            unit_obj->position.x = server_x;
            unit_obj->position.y = server_y;
            unit_obj->position.z = server_z;
            updates_this_call = 0;
            ticks_this_call = 0;
        } else {
            vehicle_obj->velocity = *(real_vector3d *)(node->vehicle_state + 0x0c);
            vehicle_obj->angular_velocity = *(real_vector3d *)(node->vehicle_state + 0x18);
            vehicle_obj->forward = *(real_vector3d *)(node->vehicle_state + 0x94);
            vehicle_obj->up = *(real_vector3d *)(node->vehicle_state + 0xa0);
            vehicle_ext->driver_seat_power = *(float *)(node->vehicle_state + 0x214);
            vehicle_ext->gunner_seat_power = *(float *)(node->vehicle_state + 0x218);
            memcpy((uint8_t *)vehicle_obj + 0x4cc, node->vehicle_state + 0x220, 0xf4);
            unit_propagate_position_delta_to_children();
            vehicle_obj->velocity = vehicle_ack->vehicle.velocity;
            vehicle_obj->angular_velocity = vehicle_ack->vehicle.angular_velocity;
            vehicle_obj->forward = vehicle_ack->vehicle.forward;
            vehicle_obj->up = vehicle_ack->vehicle.up;
            updates_this_call = 0;
            ticks_this_call = 0;
        }

        do {
            player_compute_view_forward_vector();
            unit_apply_control_block();
            remaining_ticks = node->tick_count;
            updates_this_call = updates_this_call + 1;
            if (0 < remaining_ticks) {
                ticks_this_call = ticks_this_call + remaining_ticks;
                do {
                    unit_updates_suppressed = 1;
                    if (vehicle_obj == 0) {
                        biped_update();
                        biped_update();
                    } else {
                        object_update();
                    }
                    remaining_ticks = remaining_ticks - 1;
                    unit_updates_suppressed = 0;
                } while (remaining_ticks != 0);
            }
            node = node->next;
        } while (node != 0);

        if (vehicle_obj == 0) {
            end_x = unit_obj->position.x;
            end_y = unit_obj->position.y;
            end_z = unit_obj->position.z;
        } else {
            end_x = vehicle_obj->position.x;
            end_y = vehicle_obj->position.y;
            end_z = vehicle_obj->position.z;
        }

        player_update_history_log_write(1, 0, "       Original Pos: [%f] [%f] [%f]",
            (double)original_x, (double)original_y, (double)original_z);
        player_update_history_log_write(1, 0, "Server Starting Pos: [%f] [%f] [%f]",
            (double)server_x, (double)server_y, (double)server_z);
        player_update_history_log_write(1, 0, "Client Starting Pos: [%f] [%f] [%f]",
            (double)client_start_x, (double)client_start_y, (double)client_start_z);
        player_update_history_log_write(1, 0, "         Ending Pos: [%f] [%f] [%f]",
            (double)end_x, (double)end_y, (double)end_z);
        player_update_history_log_write(1, 0, "         Difference: [%f]"); // UNSURE: vararg dropped
        player_update_history_log_write(1, 0, "        Ran updates: [%d] -> [%d], [%d] updates == [%d] ticks"); // UNSURE: varargs dropped

        if (vehicle_obj == 0) {
            end2_x = unit_obj->position.x;
            end2_y = unit_obj->position.y;
            end2_z = unit_obj->position.z;
        } else {
            end2_x = vehicle_obj->position.x;
            end2_y = vehicle_obj->position.y;
            end2_z = vehicle_obj->position.z;
        }

        // unknown_0c[0] call count, [1] cumulative updates, [2] cumulative ticks,
        // [3] last-call updates, [4] last-call ticks, [5] cumulative distance (float),
        // [6] average distance (float), [7] average ticks per call (float)
        result = history->unknown_0c[2] + ticks_this_call;
        history->unknown_0c[1] = history->unknown_0c[1] + updates_this_call;
        history->unknown_0c[2] = result;
        history->unknown_0c[4] = ticks_this_call;
        result = result / history->unknown_0c[0];
        history->unknown_0c[3] = updates_this_call;
        *(float *)&history->unknown_0c[7] = (float)result;
        dz = end2_z - original_z;
        distance = (real)sqrt((double)((end2_x - original_x) * (end2_x - original_x) +
                        (end2_y - original_y) * (end2_y - original_y) + dz * dz)) +
            *(float *)&history->unknown_0c[5];
        *(float *)&history->unknown_0c[5] = distance;
        *(float *)&history->unknown_0c[6] = distance / (float)history->unknown_0c[0];
        return result;
    }

    result = 0;
    player_update_history_log_write(1, 0, "Ignoring update [%d] due to starting_update == NULL");
    return result;
}

#if 0
Original Ghidra decompilation (0x4e6ff0), from tools/pack.py 0x4e6ff0:

int __cdecl
player_update_history_play
          (int param_1,uint param_2,float param_3,float param_4,float param_5,int param_6)

{
  int iVar1;
  uint uVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  char in_AL;
  int *piVar7;
  int iVar8;
  int in_ECX;
  float *pfVar9;
  int iVar10;
  float *pfVar11;
  int *piVar12;
  int *piVar13;
  int local_80;
  int local_78;
  int local_74;
  int local_70;
  float local_6c;
  float local_68;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;

  piVar7 = player_update_history_find_and_prune(param_1,in_ECX,in_AL);
  *(int *)(param_1 + 0xc) = *(int *)(param_1 + 0xc) + 1;
  *(undefined4 *)(param_1 + 0x18) = 0;
  *(undefined4 *)(param_1 + 0x1c) = 0;
  if (param_2 == 0xffffffff) {
    iVar10 = player_update_history_log_write("Ignoring update [%d] due to unit_index == NONE");
    if (piVar7 != (int *)0x0) {
      return iVar10;
    }
  }
  else if (piVar7 != (int *)0x0) {
    iVar10 = *(int *)(DAT_008603b0 + 0x34);
    iVar1 = *(int *)(iVar10 + 8 + (param_2 & 0xffff) * 0xc);
    uVar2 = *(uint *)(iVar1 + 0x11c);
    local_80 = 0;
    if (uVar2 != 0xffffffff) {
      iVar8 = FUN_0056cd10();
      if ((char)iVar8 != '\x01') {
        return iVar8;
      }
      if (param_6 == 0) {
        return 0;
      }
      if ((char)piVar7[10] != '\x01') {
        return param_6;
      }
      local_80 = *(int *)(iVar10 + 8 + (uVar2 & 0xffff) * 0xc);
      if (*(char *)(local_80 + 0x524) != '\0') {
        *(undefined1 *)(local_80 + 0x524) = 0;
        return local_80;
      }
    }
    if (*(char *)(iVar1 + 0x2a3) == '\x1b') {
      return local_80;
    }
    if (*(char *)(iVar1 + 0x2a3) == '\x1a') {
      return local_80;
    }
    if (local_80 == 0) {
      pfVar9 = (float *)(iVar1 + 0x5c);
      local_6c = *pfVar9;
      local_68 = *(float *)(iVar1 + 0x60);
      pfVar11 = (float *)(piVar7 + 0xc);
    }
    else {
      pfVar9 = (float *)(local_80 + 0x5c);
      local_6c = *pfVar9;
      local_68 = *(float *)(local_80 + 0x60);
      pfVar11 = (float *)(piVar7 + 0x40);
    }
    fVar3 = pfVar9[2];
    fVar4 = *pfVar11;
    fVar5 = pfVar11[1];
    fVar6 = pfVar11[2];
    *(int *)(iVar1 + 0x68) = piVar7[0xf];
    *(int *)(iVar1 + 0x6c) = piVar7[0x10];
    *(int *)(iVar1 + 0x70) = piVar7[0x11];
    *(int *)(iVar1 + 0x74) = piVar7[0x12];
    *(int *)(iVar1 + 0x78) = piVar7[0x13];
    *(int *)(iVar1 + 0x7c) = piVar7[0x14];
    *(int *)(iVar1 + 0xcc) = piVar7[0x15];
    *(int *)(iVar1 + 0xd0) = piVar7[0x16];
    *(int *)(iVar1 + 0xd4) = piVar7[0x17];
    piVar12 = piVar7 + 0x18;
    piVar13 = (int *)(iVar1 + 0x298);
    for (iVar10 = 0x12; iVar10 != 0; iVar10 = iVar10 + -1) {
      *piVar13 = *piVar12;
      piVar12 = piVar12 + 1;
      piVar13 = piVar13 + 1;
    }
    *(int *)(iVar1 + 0x34c) = piVar7[0x2a];
    *(int *)(iVar1 + 0x350) = piVar7[0x2b];
    *(int *)(iVar1 + 0x354) = piVar7[0x2c];
    *(int *)(iVar1 + 0x358) = piVar7[0x2d];
    *(int *)(iVar1 + 0x35c) = piVar7[0x2e];
    *(int *)(iVar1 + 0x360) = piVar7[0x2f];
    *(int *)(iVar1 + 0x364) = piVar7[0x30];
    *(int *)(iVar1 + 0x368) = piVar7[0x31];
    *(int *)(iVar1 + 0x36c) = piVar7[0x32];
    *(int *)(iVar1 + 0x370) = piVar7[0x33];
    *(int *)(iVar1 + 0x374) = piVar7[0x34];
    *(int *)(iVar1 + 0x378) = piVar7[0x35];
    *(int *)(iVar1 + 0x4cc) = piVar7[0x36];
    *(char *)(iVar1 + 0x503) = (char)piVar7[0x37];
    *(undefined1 *)(iVar1 + 0x501) = *(undefined1 *)((int)piVar7 + 0xdd);
    *(undefined1 *)(iVar1 + 0x502) = *(undefined1 *)((int)piVar7 + 0xde);
    *(undefined1 *)(iVar1 + 0x504) = *(undefined1 *)((int)piVar7 + 0xdf);
    *(short *)(iVar1 + 0x508) = (short)piVar7[0x38];
    *(int *)(iVar1 + 0x50c) = piVar7[0x39];
    *(int *)(iVar1 + 0x514) = piVar7[0x3a];
    *(int *)(iVar1 + 0x518) = piVar7[0x3b];
    *(int *)(iVar1 + 0x51c) = piVar7[0x3c];
    *(int *)(iVar1 + 0x520) = piVar7[0x3d];
    *(char *)(iVar1 + 0x4d0) = (char)piVar7[0x3e];
    *(undefined1 *)(iVar1 + 0x4d1) = *(undefined1 *)((int)piVar7 + 0xf9);
    *(undefined1 *)(iVar1 + 0x4d2) = *(undefined1 *)((int)piVar7 + 0xfa);
    *(int *)(iVar1 + 0x4d8) = piVar7[0x3f];
    if (local_80 == 0) {
      *(float *)(iVar1 + 0x5c) = param_3;
      *(float *)(iVar1 + 0x60) = param_4;
      *(float *)(iVar1 + 100) = param_5;
      local_74 = 0;
      local_78 = 0;
    }
    else {
      *(int *)(local_80 + 0x68) = piVar7[0x43];
      *(int *)(local_80 + 0x6c) = piVar7[0x44];
      *(int *)(local_80 + 0x70) = piVar7[0x45];
      *(int *)(local_80 + 0x8c) = piVar7[0x46];
      *(int *)(local_80 + 0x90) = piVar7[0x47];
      *(int *)(local_80 + 0x94) = piVar7[0x48];
      *(int *)(local_80 + 0x74) = piVar7[0x65];
      *(int *)(local_80 + 0x78) = piVar7[0x66];
      *(int *)(local_80 + 0x7c) = piVar7[0x67];
      *(int *)(local_80 + 0x80) = piVar7[0x68];
      *(int *)(local_80 + 0x84) = piVar7[0x69];
      *(int *)(local_80 + 0x88) = piVar7[0x6a];
      *(int *)(local_80 + 0x338) = piVar7[0xc5];
      *(int *)(local_80 + 0x33c) = piVar7[0xc6];
      piVar12 = piVar7 + 200;
      piVar13 = (int *)(local_80 + 0x4cc);
      for (iVar10 = 0x3d; iVar10 != 0; iVar10 = iVar10 + -1) {
        *piVar13 = *piVar12;
        piVar12 = piVar12 + 1;
        piVar13 = piVar13 + 1;
      }
      FUN_00570cb0();
      *(undefined4 *)(local_80 + 0x68) = *(undefined4 *)(param_6 + 0x14);
      *(undefined4 *)(local_80 + 0x6c) = *(undefined4 *)(param_6 + 0x18);
      *(undefined4 *)(local_80 + 0x70) = *(undefined4 *)(param_6 + 0x1c);
      *(undefined4 *)(local_80 + 0x8c) = *(undefined4 *)(param_6 + 0x20);
      *(undefined4 *)(local_80 + 0x90) = *(undefined4 *)(param_6 + 0x24);
      *(undefined4 *)(local_80 + 0x94) = *(undefined4 *)(param_6 + 0x28);
      *(undefined4 *)(local_80 + 0x74) = *(undefined4 *)(param_6 + 0x2c);
      *(undefined4 *)(local_80 + 0x78) = *(undefined4 *)(param_6 + 0x30);
      *(undefined4 *)(local_80 + 0x7c) = *(undefined4 *)(param_6 + 0x34);
      *(undefined4 *)(local_80 + 0x80) = *(undefined4 *)(param_6 + 0x38);
      *(undefined4 *)(local_80 + 0x84) = *(undefined4 *)(param_6 + 0x3c);
      *(undefined4 *)(local_80 + 0x88) = *(undefined4 *)(param_6 + 0x40);
      local_74 = 0;
      local_78 = 0;
    }
    do {
      FUN_00473d70();
      FUN_005639f0();
      local_70 = piVar7[1];
      local_74 = local_74 + 1;
      if (0 < local_70) {
        local_78 = local_78 + local_70;
        do {
          DAT_0071c419 = 1;
          if (local_80 == 0) {
            FUN_005625b0();
            unit_update();
          }
          else {
            object_update();
          }
          local_70 = local_70 + -1;
          DAT_0071c419 = 0;
        } while (local_70 != 0);
      }
      piVar7 = (int *)piVar7[0x105];
    } while (piVar7 != (int *)0x0);
    if (local_80 == 0) {
      local_60 = *(float *)(iVar1 + 0x5c);
      local_5c = *(float *)(iVar1 + 0x60);
      local_58 = *(float *)(iVar1 + 100);
    }
    else {
      local_60 = *(float *)(local_80 + 0x5c);
      local_5c = *(float *)(local_80 + 0x60);
      local_58 = *(float *)(local_80 + 100);
    }
    player_update_history_log_write
              ("       Original Pos: [%f] [%f] [%f]",(double)local_6c,(double)local_68,(double)fVar3
              );
    player_update_history_log_write
              ("Server Starting Pos: [%f] [%f] [%f]",(double)param_3,(double)param_4,(double)param_5
              );
    player_update_history_log_write
              ("Client Starting Pos: [%f] [%f] [%f]",(double)fVar4,(double)fVar5,(double)fVar6);
    player_update_history_log_write
              ("         Ending Pos: [%f] [%f] [%f]",(double)local_60,(double)local_5c,
               (double)local_58);
    player_update_history_log_write();
    player_update_history_log_write();
    if (local_80 == 0) {
      local_54 = *(float *)(iVar1 + 0x5c);
      local_50 = *(float *)(iVar1 + 0x60);
      local_4c = *(float *)(iVar1 + 100);
    }
    else {
      local_54 = *(float *)(local_80 + 0x5c);
      local_50 = *(float *)(local_80 + 0x60);
      local_4c = *(float *)(local_80 + 100);
    }
    iVar10 = *(int *)(param_1 + 0x14) + local_78;
    *(int *)(param_1 + 0x10) = *(int *)(param_1 + 0x10) + local_74;
    *(int *)(param_1 + 0x14) = iVar10;
    *(int *)(param_1 + 0x1c) = local_78;
    iVar10 = iVar10 / *(int *)(param_1 + 0xc);
    *(int *)(param_1 + 0x18) = local_74;
    *(float *)(param_1 + 0x28) = (float)iVar10;
    local_4c = local_4c - fVar3;
    fVar3 = SQRT((local_54 - local_6c) * (local_54 - local_6c) +
                 (local_50 - local_68) * (local_50 - local_68) + local_4c * local_4c) +
            *(float *)(param_1 + 0x20);
    *(float *)(param_1 + 0x20) = fVar3;
    *(float *)(param_1 + 0x24) = fVar3 / (float)*(int *)(param_1 + 0xc);
    return iVar10;
  }
  iVar10 = player_update_history_log_write("Ignoring update [%d] due to starting_update == NULL");
  return iVar10;
}
#endif
