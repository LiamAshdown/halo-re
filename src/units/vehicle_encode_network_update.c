// vehicle_encode_network_update  (Ghidra: FUN_005724d0; named for this rewrite)
// address 0x5724d0, size 524 bytes (0x5724d0..0x5726db, three `ret`s at 0x5726ca/0x5726d3/0x5726db)
// name confidence: 0.55   rewrite confidence: 0.7
// evidence:
//   - Ghidra reports zero callers because the only reference is data: the dword at 0x0069b624 is
//     the vehicle object_type_definition (0x0069b5b8, row 1 of object_type_definitions at
//     0x0069bfdc) column +0x6c (types/objects.h override_call_6c). The biped definition
//     (0x0069b4f0) has 0x55b440 (src/units/unit_submit_periodic_network_update.c) in the same
//     column, so this is the vehicle half of the same per-type "encode a network update" hook,
//     NOT the saved-film recorder out/phase4/units_types_notes.md guessed.
//   - The dispatcher object_type_override_call_0x6c (0x4f45b0, objdump 0x4f45fe..0x4f460e) pushes
//     (EDI object index, its own three stack arguments) and calls [def+0x6c]; its only caller,
//     0x45b723..0x45b730 in network_server_broadcast_object_type_changes, passes the buffer
//     0x00871de0, the budget 0x7ff8 and a "send the full baseline" flag. That fixes all four
//     stack parameters below.
//   - objdump 0x5724d0..0x5726dc for everything else: object_try_and_get(ECX = vehicle_index,
//     mask 2 = vehicle); hash_table_get(ESI = object_pooled_node_globals + 0xc, ECX =
//     vehicle_index) (the object network-id table, as in
//     src/networking/build_local_player_vehicle_update.c); QueryPerformanceCounter * 1000 /
//     the performance frequency at 0x006ac8f8 for a millisecond timestamp;
//     message_delta_encode_message (0x4ec940) called with EAX = buffer and EDX = bit_budget
//     (0x57267f/0x572686 reload stack parameters 2 and 3 right before the call), matching
//     src/networking/message_delta_encode_message.c's extra_eax/extra_edx.
//   - The header record's layout comes from the frame offsets: key at esp+0x20, the two
//     sequence bytes at +0x24/+0x25, is_delta at +0x26, the timestamp at +0x28; the baseline
//     record is the 0x40 bytes at esp+0x2c (flag byte, then object position 0x5c, velocity 0x68,
//     angular_velocity 0x8c, forward 0x74, up 0x80, in that order in memory).
// UNSURE: the meaning of the object flags bit 5 copied into the baseline, and of vehicle
//   0x526 (types/units.h unknown_526). The three-slot pointer array handed to the encoder is
//   reproduced slot for slot; which slot the encoder treats as "changed" versus "items" is the
//   same open question src/units/unit_submit_periodic_network_update.c records.
// NOTE (not fixed here, other files): the biped sibling unit_submit_periodic_network_update.c
//   drops the same EAX/EDX buffer/budget arguments and treats its object index as an unresolved
//   ECX read, but objdump 0x55b444 shows it is stack parameter 1 exactly as here; and
//   object_type_override_call_0x6c.c drops the three forwarded stack arguments.
// register convention: plain stack parameters, no register inputs.
//   // blam-cc: stack -> (vehicle_index, buffer, bit_budget, full_update)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

typedef struct vehicle_network_update_header {
    int32_t network_key;           // +0x00 hash_table_get result, 0 when unknown
    uint8_t unknown_526;           // +0x04 vehicle_data.unknown_526
    uint8_t update_sequence;       // +0x05 vehicle_data.network_update_sequence (0x527)
    uint8_t is_delta;              // +0x06 full_update == 0
    uint8_t pad_07;
    int32_t timestamp_milliseconds;// +0x08
} vehicle_network_update_header;   // size 0x0c

typedef struct vehicle_network_update_baseline {
    uint8_t object_flag_5;         // +0x00 (object.flags >> 5) & 1
    uint8_t pad_01[3];
    real_point3d position;         // +0x04 object 0x5c
    real_vector3d velocity;        // +0x10 object 0x68
    real_vector3d angular_velocity;// +0x1c object 0x8c
    real_vector3d forward;         // +0x28 object 0x74
    real_vector3d up;              // +0x34 object 0x80
} vehicle_network_update_baseline; // size 0x40

extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern uint8_t *object_pooled_node_globals;         // 0x00687130, object network-id hash_table at +0x0c
extern uint8_t network_client_vehicle_ack_enabled;  // 0x006894a1
extern int64_t performance_frequency;               // 0x006ac8f8/0x006ac8fc

extern uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index); // 0x56cc80, blam-cc: EAX unit_index
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, blam-cc: ESI table, ECX key
extern int32_t message_delta_encode_message(void *buffer, int32_t bit_budget, int32_t flag,
    int32_t message_type, void *changed, void *items, void *types, int32_t count,
    char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX bit_budget, rest on the stack
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac IAT
extern int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
extern int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);

// Encodes a network update for one vehicle into `buffer`: a full baseline (full_update == 1:
// header plus position/velocity/angular velocity/orientation plus the vehicle's delta record at
// 0x528) or a delta (header plus the delta record). Returns the encoded bit count, or 0 when
// nothing was written. Advances network_update_sequence (wrapping 0xff to 0) when the encoder
// wrote anything.
int32_t vehicle_encode_network_update(datum_index vehicle_index, void *buffer, int32_t bit_budget,
    int32_t full_update)
{
    object *obj;
    vehicle_data *vehicle;
    unit_data *unit;
    vehicle_network_update_header header;
    vehicle_network_update_baseline baseline;
    void *slots[3];                // esp+0x10, +0x14, +0x18 (+0x18 first holds the counter)
    large_integer counter;
    int32_t message_type;
    int32_t key;
    int32_t result;

    if (unit_any_flagged_seat_occupied(vehicle_index) && full_update != 0 &&
        network_client_vehicle_ack_enabled) {
        return 0;
    }

    obj = object_try_and_get(vehicle_index, 2);
    if (obj == 0) {
        return 0;
    }
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    key = 0;
    if (vehicle_index != (datum_index)0xffffffff) {
        key = hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), (int32_t)vehicle_index);
        if (key == -1) {
            key = 0;
        }
    }
    header.network_key = key;
    header.unknown_526 = vehicle->unknown_526;
    header.update_sequence = vehicle->network_update_sequence;
    header.is_delta = (uint8_t)(full_update == 0);

    QueryPerformanceCounter(&counter);
    header.timestamp_milliseconds = __alldiv(
        __allmul((int32_t)counter.parts.low_part, counter.parts.high_part, 1000, 0),
        (int32_t)performance_frequency, (int32_t)(performance_frequency >> 32));

    message_type = object_type_definitions[obj->type]->network_delta_message_type;

    if (full_update == 1) {
        baseline.object_flag_5 = (uint8_t)((obj->flags >> 5) & 1);
        baseline.position = obj->position;
        baseline.velocity = obj->velocity;
        baseline.angular_velocity = obj->angular_velocity;
        baseline.forward = obj->forward;
        baseline.up = obj->up;
        slots[0] = &vehicle->network_delta_sequence; // the delta record starts at 0x528
        slots[1] = &baseline;
        slots[2] = &header;
        result = message_delta_encode_message(buffer, bit_budget, 1, message_type,
            &slots[2], &slots[1], &slots[0], 1, 0);
    } else {
        slots[1] = &header;
        slots[2] = &vehicle->network_delta_sequence;
        result = message_delta_encode_message(buffer, bit_budget, 0, message_type,
            &slots[1], &slots[2], 0, 1, 0);
    }

    vehicle->unknown_524 = 0;
    if (result > 0) {
        vehicle->network_update_sequence++;
        if (vehicle->network_update_sequence >= 0xff) {  // `cmp cl,0xff; jb`: unsigned
            vehicle->network_update_sequence = 0;
        }
    }
    unit->unknown_474 = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x5724d0):

int FUN_005724d0(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  char cVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  LARGE_INTEGER *changed_offset;
  LARGE_INTEGER *items;
  int **type_offset;
  int *local_5c;
  byte *local_58;
  LARGE_INTEGER local_54;
  int local_4c;
  undefined1 local_48;
  undefined1 local_47;
  undefined1 local_46;
  undefined4 local_44;
  byte local_40 [4];
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  cVar1 = unit_any_flagged_seat_occupied();
  if (((cVar1 != '\0') && (param_4 != 0)) && (DAT_006894a1 != '\0')) {
    return 0;
  }
  iVar2 = object_try_and_get(2);
  if (iVar2 != 0) {
    local_4c = 0;
    if (param_1 != -1) {
      local_4c = hash_table_get();
      if (local_4c == -1) {
        local_4c = 0;
      }
    }
    local_47 = *(undefined1 *)(iVar2 + 0x527);
    local_48 = *(undefined1 *)(iVar2 + 0x526);
    local_46 = param_4 == 0;
    QueryPerformanceCounter(&local_54);
    uVar4 = __allmul(local_54.s.LowPart,local_54.s.HighPart,1000,0);
    local_44 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
    if (param_4 != 1) {
      type_offset = (int **)0x0;
      local_58 = (byte *)&local_4c;
      items = &local_54;
      changed_offset = (LARGE_INTEGER *)&local_58;
      local_54.s.LowPart = (DWORD)(int *)(iVar2 + 0x528);
    }
    else {
      local_40[0] = (byte)(*(uint *)(iVar2 + 0x10) >> 5) & 1;
      local_3c = *(undefined4 *)(iVar2 + 0x5c);
      local_38 = *(undefined4 *)(iVar2 + 0x60);
      local_34 = *(undefined4 *)(iVar2 + 100);
      local_30 = *(undefined4 *)(iVar2 + 0x68);
      local_2c = *(undefined4 *)(iVar2 + 0x6c);
      local_28 = *(undefined4 *)(iVar2 + 0x70);
      local_24 = *(undefined4 *)(iVar2 + 0x8c);
      local_20 = *(undefined4 *)(iVar2 + 0x90);
      local_1c = *(undefined4 *)(iVar2 + 0x94);
      local_18 = *(undefined4 *)(iVar2 + 0x74);
      local_14 = *(undefined4 *)(iVar2 + 0x78);
      local_10 = *(undefined4 *)(iVar2 + 0x7c);
      local_c = *(undefined4 *)(iVar2 + 0x80);
      local_8 = *(undefined4 *)(iVar2 + 0x84);
      local_4 = *(undefined4 *)(iVar2 + 0x88);
      local_58 = local_40;
      type_offset = &local_5c;
      items = (LARGE_INTEGER *)&local_58;
      changed_offset = &local_54;
      local_5c = (int *)(iVar2 + 0x528);
      local_54.s.LowPart = (DWORD)&local_4c;
    }
    iVar3 = message_delta_encode_message
                      ((uint)(param_4 == 1),
                       *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar2 + 0xb4)] + 0x10),
                       (int)changed_offset,(void **)items,(int)type_offset,1,'\0');
    *(undefined1 *)(iVar2 + 0x524) = 0;
    if ((0 < iVar3) &&
       (cVar1 = *(char *)(iVar2 + 0x527) + '\x01', *(char *)(iVar2 + 0x527) = cVar1, cVar1 == -1)) {
      *(undefined1 *)(iVar2 + 0x527) = 0;
    }
    *(undefined1 *)(iVar2 + 0x474) = 0;
    return iVar3;
  }
  return 0;
}
#endif
