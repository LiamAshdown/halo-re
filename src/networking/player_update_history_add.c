// player_update_history_add  (Ghidra: player_update_history_add, already named)
// address 0x4e6b50, size 973 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md; types/networking.h "player update history"
// section (player_update_history, player_update_history_node, k_network_update_history_maximum
// == 64); types/objects.h object_header/object (object_data data_array, stride 0xc, .data at
// +0x08); types/units.h unit_data (0x1f4..0x4cc) and biped_data (0x4cc..0x550); types/game.h
// player_action (0x20 bytes, the control record staged at 0x006f7ea4); the caller
// update_server_send_update (0x4ddfb0, out/phase2/networking/03.md) shows the actual call
// `player_update_history_add(update_history_ptr, tick_count)` after copying one player_action's
// worth of dwords (8) from &DAT_006f7ea4 onto the stack immediately before the call, and reading
// a single result byte back afterward -- which is what fixes the dropped stack arguments as
// (control by value, then an int32_t *out_update_id).
// register convention: EAX -> unit_index (the datum_index of the unit being snapshotted; not
// resolved to a specific caller expression, only that in_EAX is read and its low 16 bits used as
// an object_data index, exactly as every other object-index consumer in this batch does).
//   // blam-cc: EAX -> unit_index, stack -> history, tick_count, control, out_update_id
// UNSURE (load-bearing): the batch decompile drops player_update_history_log_write's two leading
// register arguments at both its call sites in this function, exactly as it does at every other
// call site in this file group; (1, 0) is carried over from the sibling call sites that already
// have disassembly-confirmed values (see player_update_client_local_player_update_from_network.c).
// UNSURE: destination bytes at node offsets 0xe2..0xe3 and 0xfb are never written by the source
// (GlobalAlloc's flag 0 does not zero the block) -- left uninitialized here too, not zeroed, per
// the "no invented behaviour" rule.
// UNSURE: player_update_history_node::control is declared as an opaque uint32_t[8] in
// types/networking.h; it is filled here with one player_action by value because the two are the
// same size (0x20 bytes) and player_action is what the caller's stack copy actually stages.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#include <string.h>

extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0
extern uint8_t player_unit_has_parent(datum_index player_handle);
    // foreign (game module), 0x477210, blam-cc: ECX -> player_handle. UNSURE (load-bearing):
    // this call site's decompile shows zero arguments (`cVar1 = FUN_00477210();`), i.e. the
    // batch dropped whatever ECX held; unit_ext->controlling_player (unit_data +0x218) is the
    // only player handle in scope and is what is passed here, but this is a reconstruction, not
    // something the decompile itself shows.

// Allocates and appends one snapshot of a unit's (and, if seated, its vehicle's) state onto a
// player_update_history list, keyed by the list's own rolling update id. Refuses once the list
// already holds 64 nodes, logging the overflow and reporting -1 through *out_update_id instead.
uint8_t player_update_history_add(datum_index unit_index, player_update_history *history,
    int32_t tick_count, player_action control, int32_t *out_update_id)
    // blam-cc: EAX -> unit_index, stack -> history, tick_count, control, out_update_id
{
    player_update_history_node *node;
    player_update_history_node *walk;
    object *unit_obj;
    object *vehicle_obj;
    unit_data *unit_ext;
    biped_data *biped_ext;
    unit_data *vehicle_ext;
    int32_t count;
    int32_t tick_sum;
    uint32_t next_id;

    count = 0;
    walk = history->head;
    if (walk != 0) {
        do {
            walk = walk->next;
            count = count + 1;
        } while (walk != 0);
        if (0x3f < count) {
            count = 0;
            tick_sum = 0;
            for (walk = history->head; walk != 0; walk = walk->next) {
                count = count + 1;
                tick_sum = tick_sum + walk->tick_count;
            }
            player_update_history_log_write(1, 0,
                "[%d]: Player update history overflow, [%d] updates == [%d] ticks.\n",
                game_time->game_time, count, tick_sum);
            *out_update_id = -1;
            return 0;
        }
    }

    node = (player_update_history_node *)GlobalAlloc(0, sizeof(player_update_history_node));
    node->update_id = history->next_update_id;
    node->tick_count = tick_count;
    memcpy(node->control, &control, sizeof(node->control));
    node->next = 0;

    next_id = (history->next_update_id + 1) & 0x8000003f;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xffffffc0) + 1;
    }
    history->next_update_id = next_id;

    unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_ext = (unit_data *)((uint8_t *)unit_obj + 0x1f4);
    biped_ext = (biped_data *)((uint8_t *)unit_obj + 0x4cc);

    node->vehicle_object = unit_obj->parent_object;

    // unit_state is a repacked (not contiguous-source) snapshot of the fields below; the
    // destination layout matches the original dword-by-dword assignments exactly.
    *(real_point3d *)(node->unit_state + 0x00) = unit_obj->position;
    *(real_vector3d *)(node->unit_state + 0x0c) = unit_obj->velocity;
    *(real_vector3d *)(node->unit_state + 0x18) = unit_obj->forward;
    *(datum_index *)(node->unit_state + 0x24) = unit_obj->animation_graph;
    *(int16_t *)(node->unit_state + 0x28) = unit_obj->animation_index;
    *(int16_t *)(node->unit_state + 0x2a) = unit_obj->animation_frame;
    *(int16_t *)(node->unit_state + 0x2c) = unit_obj->interpolation_frame_index;
    *(int16_t *)(node->unit_state + 0x2e) = unit_obj->node_function_count;
    memcpy(node->unit_state + 0x30, &unit_ext->animation_state_flags, 0x48); // unit 0x298..0x2e0
    memcpy(node->unit_state + 0x78, &unit_ext->seat_acceleration_last_position, 0x30);           // unit 0x34c..0x37c
    *(uint32_t *)(node->unit_state + 0xa8) = biped_ext->flags;
    node->unit_state[0xac] = biped_ext->stop_moving_ticks;
    node->unit_state[0xad] = biped_ext->airborne_ticks;
    node->unit_state[0xae] = biped_ext->slipping_ticks;
    node->unit_state[0xaf] = biped_ext->jump_ticks;
    *(int16_t *)(node->unit_state + 0xb0) = biped_ext->landing_type; // 0xb2..0xb3 left uninitialized
    *(float *)(node->unit_state + 0xb4) = biped_ext->crouch_fraction;
    *(real_vector3d *)(node->unit_state + 0xb8) = biped_ext->ground_normal;
    *(uint32_t *)(node->unit_state + 0xc4) = biped_ext->ground_plane_distance;
    node->unit_state[0xc8] = biped_ext->landing_ticks;
    node->unit_state[0xc9] = biped_ext->landing_duration_ticks;
    node->unit_state[0xca] = biped_ext->movement_state; // 0xcb left uninitialized
    *(datum_index *)(node->unit_state + 0xcc) = biped_ext->ground_surface_index;

    if (player_unit_has_parent(unit_ext->controlling_player)) {
        vehicle_obj = ((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data;
        node->has_vehicle = 1;
        *(real_point3d *)(node->vehicle_state + 0x00) = vehicle_obj->position;
        *(real_vector3d *)(node->vehicle_state + 0x0c) = vehicle_obj->velocity;
        *(real_vector3d *)(node->vehicle_state + 0x18) = vehicle_obj->angular_velocity;
        memcpy(node->vehicle_state + 0x24, (uint8_t *)vehicle_obj + 0x04, 0x1f0); // rest of object
        vehicle_ext = (unit_data *)((uint8_t *)vehicle_obj + 0x1f4);
        *(float *)(node->vehicle_state + 0x214) = vehicle_ext->driver_seat_power;
        *(float *)(node->vehicle_state + 0x218) = vehicle_ext->gunner_seat_power;
        *(uint32_t *)(node->vehicle_state + 0x21c) = 0;
        memcpy(node->vehicle_state + 0x220, (uint8_t *)vehicle_obj + 0x4cc, 0xf4); // vehicle_data
    } else {
        node->has_vehicle = 0;
    }

    if (history->tail != 0) {
        history->tail->next = node;
    }
    history->tail = node;
    if (history->head == 0) {
        history->head = node;
    }

    count = 0;
    tick_sum = 0;
    for (walk = history->head; walk != 0; walk = walk->next) {
        count = count + 1;
        tick_sum = tick_sum + walk->tick_count;
    }
    if (node->update_id % 10 == 0) {
        player_update_history_log_write(1, 0,
            "[%d]: Added through update [%d]. [%d]/[%d]updates == [%d] ticks\n",
            game_time->game_time, node->update_id, count,
            0x40, tick_sum);
    }
    if (count == 0x40) {
        player_update_history_log_write(1, 0,
            "[%d]: Warning...Update history is now full, [%d] updates.\n",
            game_time->game_time, 0x40);
    }

    *out_update_id = node->update_id;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e6b50), from tools/pack.py 0x4e6b50:

uint player_update_history_add(uint *param_1,uint param_2)

{
  char cVar1;
  uint in_EAX;
  uint *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  uint *puVar7;
  uint *puVar8;
  uint *puVar9;
  uint *in_stack_0000002c;

  uVar6 = param_1[1];
  iVar5 = 0;
  uVar3 = uVar6;
  if (uVar6 != 0) {
    do {
      uVar3 = *(uint *)(uVar3 + 0x414);
      iVar5 = iVar5 + 1;
    } while (uVar3 != 0);
    if (0x3f < iVar5) {
      iVar5 = 0;
      iVar4 = 0;
      for (; uVar6 != 0; uVar6 = *(uint *)(uVar6 + 0x414)) {
        iVar5 = iVar5 + 1;
        iVar4 = iVar4 + *(int *)(uVar6 + 4);
      }
      uVar6 = player_update_history_log_write
                        ("[%d]: Player update history overflow, [%d] updates == [%d] ticks.\n",
                         *(undefined4 *)(DAT_006f1d6c + 0xc),iVar5,iVar4);
      *in_stack_0000002c = 0xffffffff;
      return uVar6 & 0xffffff00;
    }
  }
  puVar2 = GlobalAlloc(0,0x418);
  uVar6 = *param_1;
  puVar2[1] = param_2;
  *puVar2 = uVar6;
  puVar7 = (uint *)&stack0x0000000c;
  puVar9 = puVar2 + 2;
  for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar9 = *puVar7;
    puVar7 = puVar7 + 1;
    puVar9 = puVar9 + 1;
  }
  puVar2[0x105] = 0;
  uVar6 = *param_1 + 1 & 0x8000003f;
  if ((int)uVar6 < 0) {
    uVar6 = (uVar6 - 1 | 0xffffffc0) + 1;
  }
  *param_1 = uVar6;
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  puVar2[0xb] = *(uint *)(iVar5 + 0x11c);
  puVar2[0xc] = *(uint *)(iVar5 + 0x5c);
  puVar2[0xd] = *(uint *)(iVar5 + 0x60);
  puVar2[0xe] = *(uint *)(iVar5 + 100);
  puVar2[0xf] = *(uint *)(iVar5 + 0x68);
  puVar2[0x10] = *(uint *)(iVar5 + 0x6c);
  puVar2[0x11] = *(uint *)(iVar5 + 0x70);
  puVar2[0x12] = *(uint *)(iVar5 + 0x74);
  puVar2[0x13] = *(uint *)(iVar5 + 0x78);
  puVar2[0x14] = *(uint *)(iVar5 + 0x7c);
  puVar2[0x15] = *(uint *)(iVar5 + 0xcc);
  puVar2[0x16] = *(uint *)(iVar5 + 0xd0);
  puVar2[0x17] = *(uint *)(iVar5 + 0xd4);
  puVar7 = (uint *)(iVar5 + 0x298);
  puVar9 = puVar2 + 0x18;
  for (iVar4 = 0x12; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar9 = *puVar7;
    puVar7 = puVar7 + 1;
    puVar9 = puVar9 + 1;
  }
  puVar2[0x2a] = *(uint *)(iVar5 + 0x34c);
  puVar2[0x2b] = *(uint *)(iVar5 + 0x350);
  puVar2[0x2c] = *(uint *)(iVar5 + 0x354);
  puVar2[0x2d] = *(uint *)(iVar5 + 0x358);
  puVar2[0x2e] = *(uint *)(iVar5 + 0x35c);
  puVar2[0x2f] = *(uint *)(iVar5 + 0x360);
  puVar2[0x30] = *(uint *)(iVar5 + 0x364);
  puVar2[0x31] = *(uint *)(iVar5 + 0x368);
  puVar2[0x32] = *(uint *)(iVar5 + 0x36c);
  puVar2[0x33] = *(uint *)(iVar5 + 0x370);
  puVar2[0x34] = *(uint *)(iVar5 + 0x374);
  puVar2[0x35] = *(uint *)(iVar5 + 0x378);
  puVar2[0x36] = *(uint *)(iVar5 + 0x4cc);
  *(undefined1 *)(puVar2 + 0x37) = *(undefined1 *)(iVar5 + 0x503);
  *(undefined1 *)((int)puVar2 + 0xdd) = *(undefined1 *)(iVar5 + 0x501);
  *(undefined1 *)((int)puVar2 + 0xde) = *(undefined1 *)(iVar5 + 0x502);
  *(undefined1 *)((int)puVar2 + 0xdf) = *(undefined1 *)(iVar5 + 0x504);
  *(undefined2 *)(puVar2 + 0x38) = *(undefined2 *)(iVar5 + 0x508);
  puVar2[0x39] = *(uint *)(iVar5 + 0x50c);
  puVar2[0x3a] = *(uint *)(iVar5 + 0x514);
  puVar2[0x3b] = *(uint *)(iVar5 + 0x518);
  puVar2[0x3c] = *(uint *)(iVar5 + 0x51c);
  puVar2[0x3d] = *(uint *)(iVar5 + 0x520);
  *(undefined1 *)(puVar2 + 0x3e) = *(undefined1 *)(iVar5 + 0x4d0);
  *(undefined1 *)((int)puVar2 + 0xf9) = *(undefined1 *)(iVar5 + 0x4d1);
  *(undefined1 *)((int)puVar2 + 0xfa) = *(undefined1 *)(iVar5 + 0x4d2);
  puVar2[0x3f] = *(uint *)(iVar5 + 0x4d8);
  cVar1 = FUN_00477210();
  if (cVar1 == '\x01') {
    puVar7 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar5 + 0x11c) & 0xffff) * 0xc);
    *(undefined1 *)(puVar2 + 10) = 1;
    puVar2[0x40] = puVar7[0x17];
    puVar2[0x41] = puVar7[0x18];
    puVar2[0x42] = puVar7[0x19];
    puVar2[0x43] = puVar7[0x1a];
    puVar2[0x44] = puVar7[0x1b];
    puVar2[0x45] = puVar7[0x1c];
    puVar2[0x46] = puVar7[0x23];
    puVar2[0x47] = puVar7[0x24];
    puVar2[0x48] = puVar7[0x25];
    puVar8 = puVar2 + 0x49;
    puVar9 = puVar7;
    for (iVar5 = 0x7c; puVar9 = puVar9 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar8 = *puVar9;
      puVar8 = puVar8 + 1;
    }
    puVar2[0xc5] = puVar7[0xce];
    puVar2[0xc6] = puVar7[0xcf];
    puVar2[199] = 0;
    puVar7 = puVar7 + 0x133;
    puVar9 = puVar2 + 200;
    for (iVar5 = 0x3d; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar9 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar9 = puVar9 + 1;
    }
  }
  else {
    *(undefined1 *)(puVar2 + 10) = 0;
  }
  if (param_1[2] != 0) {
    *(uint **)(param_1[2] + 0x414) = puVar2;
  }
  param_1[2] = (uint)puVar2;
  if (param_1[1] == 0) {
    param_1[1] = (uint)puVar2;
  }
  iVar5 = 0;
  iVar4 = 0;
  for (uVar6 = param_1[1]; uVar6 != 0; uVar6 = *(uint *)(uVar6 + 0x414)) {
    iVar5 = iVar5 + 1;
    iVar4 = iVar4 + *(int *)(uVar6 + 4);
  }
  if ((int)*puVar2 % 10 == 0) {
    player_update_history_log_write
              ("[%d]: Added through update [%d]. [%d]/[%d]updates == [%d] ticks\n",
               *(undefined4 *)(DAT_006f1d6c + 0xc),*puVar2,iVar5,0x40,iVar4);
  }
  if (iVar5 == 0x40) {
    player_update_history_log_write
              ("[%d]: Warning...Update history is now full, [%d] updates.\n",
               *(undefined4 *)(DAT_006f1d6c + 0xc),0x40);
  }
  uVar6 = *puVar2;
  *in_stack_0000002c = uVar6;
  return CONCAT31((int3)(uVar6 >> 8),1);
}
#endif
