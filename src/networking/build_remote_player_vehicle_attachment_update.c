// build_remote_player_vehicle_attachment_update  (Ghidra: FUN_004e86f0; named per this rewrite)
// address 0x4e86f0, size 709 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Builds or resends a cached secondary
// vehicle-attachment transform update (message 0x2a) for a given object record."); near-identical
// structure to build_remote_player_vehicle_update.c (this batch, message 0x29), with an added
// object_data lookup chasing param_1+0x34 (a unit) to its parent object (its vehicle) before
// reading that vehicle's transform -- matching build_local_player_vehicle_update.c's identical
// chase of the same offsets.
// register convention: EBX -> control (a player_action-shaped control record), ECX -> network_key
// (the primary object's raw datum index), stack -> cache, update_id, flags, is_full.
//   // blam-cc: EBX -> control, ECX -> network_key, stack -> cache, update_id, flags, is_full
// UNSURE (extensive): cache's type is not established anywhere in this batch's headers, same as
// build_remote_player_vehicle_update.c; every offset is preserved exactly as Ghidra shows rather
// than guessing a struct. flags (param_3) is written into the staged buffer's first byte and then
// immediately overwritten by the control-record copy, a dead store preserved rather than removed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sin(double x); // FSIN
extern double cos(double x); // FCOS

extern data_array *object_data; // 0x008603b0
extern void *machine_table; // 0x00687558, see build_remote_player_vehicle_update.c
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size

// Builds a message-0x2a (remote-player secondary/attached-vehicle transform) update for cache
// from control. Like build_remote_player_vehicle_update, stages a direction vector from control's
// yaw/pitch, then chases cache's unit (+0x34) to its parent vehicle object and stages that
// vehicle's own transform. When is_full is set, encodes the full record and refreshes cache's
// last-sent bookkeeping (offsets 0x17c..0x188); otherwise encodes a delta against the previously
// cached transform.
void build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)
    // blam-cc: EBX -> control, ECX -> network_key, stack -> cache, update_id, flags, is_full
{
    int32_t network_hash;
    uint32_t staged[12]; // control[8] + uninitialized[1] + direction[3]
    uint8_t staged_update_id;
    uint32_t next_id;
    real direction_x, direction_y, direction_z;
    int32_t i;
    object *unit_obj;
    object *vehicle_obj;
    datum_index parent_object;
    int32_t vehicle_hash;
    uint32_t vehicle_state[16]; // vehicle_hash, position[3], transform[12]
    int32_t previous_state[12];
    void *items_ptr;
    void *previous_ptr;

    staged_update_id = update_id;
    network_hash = 0;
    if (network_key != -1) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), network_key);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ((uint8_t *)staged)[0] = flags; // dead store: overwritten by the copy below
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)control)[i];
    }
    // staged[8] is never written here (matches the original's undersized 9-dword local_e0,
    // whose 9th dword is genuinely uninitialized stack content); left unset deliberately.
    direction_x = (real)(cos((double)control->desired_pitch) * cos((double)control->desired_yaw));
    direction_y = (real)(cos((double)control->desired_pitch) * sin((double)control->desired_yaw));
    direction_z = (real)sin((double)control->desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;

    unit_obj = ((object_header *)object_data->data)[*(uint32_t *)(cache + 0x34) & 0xffff].data;
    parent_object = unit_obj->parent_object;
    vehicle_obj = ((object_header *)object_data->data)[parent_object & 0xffff].data;
    vehicle_hash = 0;
    if (parent_object != (datum_index)-1) {
        vehicle_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), parent_object);
        if (vehicle_hash == -1) {
            vehicle_hash = 0;
        }
    }
    vehicle_state[0] = vehicle_hash;
    vehicle_state[1] = *(uint32_t *)(cache + 0xf8);
    vehicle_state[2] = *(uint32_t *)(cache + 0xfc);
    vehicle_state[3] = *(uint32_t *)(cache + 0x100);
    *(real_vector3d *)&vehicle_state[4] = vehicle_obj->velocity;
    *(real_vector3d *)&vehicle_state[7] = vehicle_obj->angular_velocity;
    *(real_vector3d *)&vehicle_state[10] = vehicle_obj->forward;
    *(real_vector3d *)&vehicle_state[13] = vehicle_obj->up;

    if (is_full == '\x01') {
        // The whole 12-dword staged buffer (control, the uninitialized gap, and direction) is
        // copied into cache+0x130 verbatim, exactly as Ghidra's own 12-dword loop does.
        for (i = 0; i < 12; i = i + 1) {
            *(uint32_t *)(cache + 0x130 + i * 4) = staged[i];
        }

        *(int32_t *)(cache + 0x124) = game_time->game_time;
        next_id = (*(uint8_t *)(cache + 300) + 1) & 0x80000001;
        *(int32_t *)(cache + 0x120) = game_time->game_time;
        *(uint32_t *)(cache + 0x128) = update_id;
        if ((int32_t)next_id < 0) {
            next_id = (next_id - 1 | 0xfffffffe) + 1;
        }
        staged_update_id = (uint8_t)next_id;
        *(uint8_t *)(cache + 300) = (uint8_t)next_id;
        previous_ptr = &network_hash;
        items_ptr = staged;
        message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x2a, (int32_t)&previous_ptr, &items_ptr, 0, 1, '\0');
        for (i = 0; i < 16; i = i + 1) {
            *(uint32_t *)(cache + 0x188 + i * 4) = vehicle_state[i];
        }
        *(int32_t *)(cache + 0x180) = game_time->game_time;
        *(uint32_t *)(cache + 0x184) = staged_update_id;
        *(int32_t *)(cache + 0x17c) = game_time->game_time;
        return;
    }

    for (i = 0; i < 12; i = i + 1) {
        previous_state[i] = *(int32_t *)(cache + 0x130 + i * 4);
    }
    items_ptr = staged;
    previous_ptr = previous_state;
    message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x2a, (int32_t)&previous_ptr, &items_ptr,
        (int32_t)vehicle_state, 1, '\x01');
    *(int32_t *)(cache + 0x120) = game_time->game_time;
    *(int32_t *)(cache + 0x17c) = game_time->game_time;
}

#if 0
Original Ghidra decompilation (0x4e86f0), from tools/pack.py 0x4e86f0:

void FUN_004e86f0(int param_1,byte param_2,undefined1 param_3,char param_4)

{
  undefined4 uVar1;
  int iVar2;
  int in_ECX;
  int iVar3;
  uint uVar4;
  undefined4 *unaff_EBX;
  undefined4 *puVar5;
  int *piVar6;
  undefined4 *puVar7;
  int *piVar8;
  float10 fVar9;
  float10 fVar10;
  int *local_f8;
  undefined4 *local_f4;
  int local_f0;
  byte local_ec;
  undefined1 local_eb;
  undefined1 local_e8;
  int *local_e4;
  undefined4 local_e0 [9];
  float local_bc;
  float local_b8;
  float local_b4;
  int local_b0 [4];
  undefined4 local_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined4 local_88;
  undefined4 local_84;
  undefined4 local_80;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_74;
  int local_70 [12];
  undefined4 local_40 [16];

  local_ec = param_2;
  local_f0 = 0;
  if (in_ECX != -1) {
    local_f0 = hash_table_get();
    if (local_f0 == -1) {
      local_f0 = 0;
    }
  }
  local_eb = *(undefined1 *)(param_1 + 300);
  fVar9 = (float10)fcos((float10)(float)unaff_EBX[2]);
  local_e0[0]._0_1_ = param_3;
  puVar7 = local_e0;
  puVar5 = unaff_EBX;
  for (iVar3 = 8; puVar7 = puVar7 + 1, iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar7 = *puVar5;
    puVar5 = puVar5 + 1;
  }
  local_e8 = *(undefined1 *)(param_1 + 0x160);
  fVar10 = (float10)fcos((float10)(float)unaff_EBX[1]);
  local_bc = (float)(fVar10 * fVar9);
  fVar10 = (float10)fsin((float10)(float)unaff_EBX[1]);
  local_b8 = (float)(fVar10 * fVar9);
  fVar9 = (float10)fsin((float10)(float)unaff_EBX[2]);
  local_b4 = (float)fVar9;
  uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                            (*(uint *)(param_1 + 0x34) & 0xffff) * 0xc) + 0x11c);
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
  local_b0[0] = 0;
  if (uVar4 != 0xffffffff) {
    local_b0[0] = hash_table_get();
    if (local_b0[0] == -1) {
      local_b0[0] = 0;
    }
  }
  iVar2 = DAT_006f1d6c;
  local_b0[1] = *(undefined4 *)(param_1 + 0xf8);
  local_b0[2] = *(undefined4 *)(param_1 + 0xfc);
  local_b0[3] = *(undefined4 *)(param_1 + 0x100);
  local_a0 = *(undefined4 *)(iVar3 + 0x68);
  local_9c = *(undefined4 *)(iVar3 + 0x6c);
  local_98 = *(undefined4 *)(iVar3 + 0x70);
  local_94 = *(undefined4 *)(iVar3 + 0x8c);
  local_90 = *(undefined4 *)(iVar3 + 0x90);
  local_8c = *(undefined4 *)(iVar3 + 0x94);
  local_88 = *(undefined4 *)(iVar3 + 0x74);
  local_84 = *(undefined4 *)(iVar3 + 0x78);
  local_80 = *(undefined4 *)(iVar3 + 0x7c);
  local_7c = *(undefined4 *)(iVar3 + 0x80);
  local_78 = *(undefined4 *)(iVar3 + 0x84);
  local_74 = *(undefined4 *)(iVar3 + 0x88);
  iVar3 = 0xc;
  if (param_4 == '\x01') {
    puVar7 = local_e0;
    puVar5 = (undefined4 *)(param_1 + 0x130);
    for (; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined4 *)(param_1 + 0x124) = *(undefined4 *)(iVar2 + 0xc);
    uVar4 = *(int *)(param_1 + 300) + 1U & 0x80000001;
    *(undefined4 *)(param_1 + 0x120) = *(undefined4 *)(iVar2 + 0xc);
    *(uint *)(param_1 + 0x128) = (uint)param_2;
    if ((int)uVar4 < 0) {
      uVar4 = (uVar4 - 1 | 0xfffffffe) + 1;
    }
    local_eb = (undefined1)uVar4;
    local_f4 = local_e0;
    *(uint *)(param_1 + 300) = uVar4;
    local_f8 = &local_f0;
    message_delta_encode_message(0,0x2a,(int)&local_f8,&local_f4,0,1,'\0');
    piVar6 = local_b0;
    piVar8 = (int *)(param_1 + 0x188);
    for (iVar3 = 0x10; iVar2 = DAT_006f1d6c, iVar3 != 0; iVar3 = iVar3 + -1) {
      *piVar8 = *piVar6;
      piVar6 = piVar6 + 1;
      piVar8 = piVar8 + 1;
    }
    *(undefined4 *)(param_1 + 0x180) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    uVar1 = *(undefined4 *)(iVar2 + 0xc);
    *(uint *)(param_1 + 0x184) = (uint)local_ec;
    *(undefined4 *)(param_1 + 0x17c) = uVar1;
    return;
  }
  piVar6 = (int *)(param_1 + 0x130);
  piVar8 = local_70;
  for (; iVar3 != 0; iVar3 = iVar3 + -1) {
    *piVar8 = *piVar6;
    piVar6 = piVar6 + 1;
    piVar8 = piVar8 + 1;
  }
  puVar7 = (undefined4 *)(param_1 + 0x188);
  puVar5 = local_40;
  for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar7;
    puVar7 = puVar7 + 1;
    puVar5 = puVar5 + 1;
  }
  local_f8 = local_70;
  local_f4 = local_e0;
  local_e4 = &local_f0;
  message_delta_encode_message(1,0x2a,(int)&local_e4,&local_f4,(int)&local_f8,1,'\x01');
  iVar3 = DAT_006f1d6c;
  *(undefined4 *)(param_1 + 0x120) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  *(undefined4 *)(param_1 + 0x17c) = *(undefined4 *)(iVar3 + 0xc);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
