// build_remote_player_vehicle_update  (Ghidra: build_remote_player_vehicle_update, already named)
// address 0x4e84d0, size 538 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Builds or resends a cached vehicle-transform
// update (message 0x29) for a given object record."); types/game.h player_action (0x20 bytes,
// matches the 8-dword copy from unaff_EBX); types/projectiles.h network-records section
// (hash_table_get against object_pooled_node_globals+0x0c, -1 written as 0).
// register convention: EBX -> control (a player_action-shaped control record), ECX -> network_key
// (the object's raw datum index, hashed for its network id), stack -> cache, update_id, flags,
// is_full.
//   // blam-cc: EBX -> control, ECX -> network_key, stack -> cache, update_id, flags, is_full
// UNSURE (extensive): cache's type is not established anywhere in this batch's headers -- it is
// some per-object broadcast-cache record (offsets 0xf8..0x178) that this batch's types notes do
// not attribute a struct to. It is left as a raw byte pointer with every offset preserved exactly
// as Ghidra shows. The staged encode buffer's layout (an 8-dword control copy, a computed
// direction vector, three raw position dwords, then 12 more raw dwords) matches the size and
// shape of a wire vehicle-update record but is not matched against any declared type, since doing
// so would require re-deriving cache's own layout first. flags (param_3) is written into the
// first byte of the staged buffer and then immediately overwritten by the control-record copy
// that follows it -- a dead store preserved here rather than removed.
// UNSURE: message_delta_encode_message's changed_offset/type_offset arguments are pointers here
// (cast to int32_t), consistent with other callers in this batch that pass a "previous state"
// pointer through the same int32_t-typed parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

extern double sin(double x); // FSIN
extern double cos(double x); // FCOS

extern uint8_t *object_pooled_node_globals; // 0x00687130 (this batch calls the +0xc hash table
    // through this global elsewhere; this function's own hash table pointer, PTR_DAT_00687558,
    // is a different global and is declared separately below)
extern void *remote_player_index_remap_table; // 0x00687558, see types/networking.h
    // remote_player_update_header's header comment; UNSURE: reused here as the network-id hash
    // table by analogy with the other hash_table_get call sites in this batch, not re-derived
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size

// Builds a message-0x29 (remote-player vehicle transform) update for cache from control (the
// object's current control/aim record). When is_full is set, stages a full record (control plus
// a direction vector computed from control's yaw/pitch, plus cache's raw position and transform
// fields), encodes it, and refreshes cache's last-sent bookkeeping. Otherwise stages a delta
// against cache's previously-sent transform fields and encodes that instead.
void build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key)
    // blam-cc: EBX -> control, ECX -> network_key, stack -> cache, update_id, flags, is_full
{
    int32_t network_hash;
    uint32_t staged[27]; // control[8] + direction[3] + position[3] + transform[12]
    uint8_t staged_update_id;
    uint32_t next_id;
    real direction_x, direction_y, direction_z;
    int32_t i;
    int32_t previous_state[15];
    void *items_ptr;
    void *previous_ptr;

    staged_update_id = update_id;
    network_hash = 0;
    if (network_key != -1) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)remote_player_index_remap_table + 0x0c), network_key);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ((uint8_t *)staged)[0] = flags; // dead store: overwritten by the copy below
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)control)[i];
    }
    // staged[8] is never written here (matches the original's undersized 9-dword local_78,
    // whose 9th dword is genuinely uninitialized stack content); left unset deliberately.
    direction_x = (real)(cos((double)control->desired_pitch) * cos((double)control->desired_yaw));
    direction_y = (real)(cos((double)control->desired_pitch) * sin((double)control->desired_yaw));
    direction_z = (real)sin((double)control->desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;
    staged[12] = *(uint32_t *)(cache + 0xf8);
    staged[13] = *(uint32_t *)(cache + 0xfc);
    staged[14] = *(uint32_t *)(cache + 0x100);

    if (is_full == '\x01') {
        for (i = 0; i < 12; i = i + 1) {
            staged[15 + i] = *(uint32_t *)(cache + 0x130 + i * 4);
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
        message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x29, (int32_t)&previous_ptr, &items_ptr, 0, 1, '\0');
        *(uint32_t *)(cache + 0x170) = staged[12];
        *(uint32_t *)(cache + 0x174) = staged[13];
        *(uint32_t *)(cache + 0x178) = staged[14];
        *(int32_t *)(cache + 0x168) = game_time->game_time;
        *(uint32_t *)(cache + 0x16c) = staged_update_id;
        *(int32_t *)(cache + 0x164) = game_time->game_time;
        return;
    }

    for (i = 0; i < 12; i = i + 1) {
        previous_state[i] = *(int32_t *)(cache + 0x130 + i * 4);
    }
    previous_state[12] = *(int32_t *)(cache + 0x170);
    previous_state[13] = *(int32_t *)(cache + 0x174);
    previous_state[14] = *(int32_t *)(cache + 0x178);
    items_ptr = staged;
    previous_ptr = previous_state;
    message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x29, (int32_t)&previous_ptr, &items_ptr,
        (int32_t)&previous_state, 1, '\x01');
    *(int32_t *)(cache + 0x120) = game_time->game_time;
    *(int32_t *)(cache + 0x164) = game_time->game_time;
}

#if 0
Original Ghidra decompilation (0x4e84d0), from tools/pack.py 0x4e84d0:

void build_remote_player_vehicle_update(int param_1,byte param_2,undefined1 param_3,char param_4)

{
  undefined4 uVar1;
  int iVar2;
  int in_ECX;
  int iVar3;
  uint uVar4;
  undefined4 *unaff_EBX;
  undefined4 *puVar5;
  undefined4 *puVar6;
  int *piVar7;
  int *piVar8;
  float10 fVar9;
  float10 fVar10;
  int *local_90;
  undefined4 *local_8c;
  int local_88;
  byte local_84;
  undefined1 local_83;
  undefined1 local_80;
  int *local_7c;
  undefined4 local_78 [9];
  float local_54;
  float local_50;
  float local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  int local_3c [12];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_84 = param_2;
  local_88 = 0;
  if (in_ECX != -1) {
    local_88 = hash_table_get();
    if (local_88 == -1) {
      local_88 = 0;
    }
  }
  iVar2 = DAT_006f1d6c;
  local_83 = *(undefined1 *)(param_1 + 300);
  fVar9 = (float10)fcos((float10)(float)unaff_EBX[2]);
  local_78[0]._0_1_ = param_3;
  puVar6 = local_78;
  puVar5 = unaff_EBX;
  for (iVar3 = 8; puVar6 = puVar6 + 1, iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
  }
  local_80 = *(undefined1 *)(param_1 + 0x160);
  local_44 = *(undefined4 *)(param_1 + 0xfc);
  local_40 = *(undefined4 *)(param_1 + 0x100);
  local_48 = *(undefined4 *)(param_1 + 0xf8);
  iVar3 = 0xc;
  fVar10 = (float10)fcos((float10)(float)unaff_EBX[1]);
  local_54 = (float)(fVar10 * fVar9);
  fVar10 = (float10)fsin((float10)(float)unaff_EBX[1]);
  local_50 = (float)(fVar10 * fVar9);
  fVar9 = (float10)fsin((float10)(float)unaff_EBX[2]);
  local_4c = (float)fVar9;
  if (param_4 == '\x01') {
    puVar6 = local_78;
    puVar5 = (undefined4 *)(param_1 + 0x130);
    for (; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined4 *)(param_1 + 0x124) = *(undefined4 *)(iVar2 + 0xc);
    uVar4 = *(int *)(param_1 + 300) + 1U & 0x80000001;
    *(undefined4 *)(param_1 + 0x120) = *(undefined4 *)(iVar2 + 0xc);
    *(uint *)(param_1 + 0x128) = (uint)param_2;
    if ((int)uVar4 < 0) {
      uVar4 = (uVar4 - 1 | 0xfffffffe) + 1;
    }
    local_83 = (undefined1)uVar4;
    local_8c = local_78;
    *(uint *)(param_1 + 300) = uVar4;
    local_90 = &local_88;
    message_delta_encode_message(0,0x29,(int)&local_90,&local_8c,0,1,'\0');
    *(undefined4 *)(param_1 + 0x170) = local_48;
    *(undefined4 *)(param_1 + 0x174) = local_44;
    *(undefined4 *)(param_1 + 0x178) = local_40;
    iVar2 = DAT_006f1d6c;
    *(undefined4 *)(param_1 + 0x168) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    uVar1 = *(undefined4 *)(iVar2 + 0xc);
    *(uint *)(param_1 + 0x16c) = (uint)local_84;
    *(undefined4 *)(param_1 + 0x164) = uVar1;
    return;
  }
  piVar7 = (int *)(param_1 + 0x130);
  piVar8 = local_3c;
  for (; iVar3 != 0; iVar3 = iVar3 + -1) {
    *piVar8 = *piVar7;
    piVar7 = piVar7 + 1;
    piVar8 = piVar8 + 1;
  }
  local_c = *(undefined4 *)(param_1 + 0x170);
  local_8 = *(undefined4 *)(param_1 + 0x174);
  local_4 = *(undefined4 *)(param_1 + 0x178);
  local_90 = local_3c;
  local_8c = local_78;
  local_7c = &local_88;
  message_delta_encode_message(1,0x29,(int)&local_7c,&local_8c,(int)&local_90,1,'\x01');
  iVar2 = DAT_006f1d6c;
  *(undefined4 *)(param_1 + 0x120) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  *(undefined4 *)(param_1 + 0x164) = *(undefined4 *)(iVar2 + 0xc);
  return;
}
#endif
