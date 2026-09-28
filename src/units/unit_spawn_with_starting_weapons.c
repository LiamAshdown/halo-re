// unit_spawn_with_starting_weapons  (Ghidra: FUN_00572110)
// address 0x572110, size 744 bytes, name confidence 0.2 (really the vehicle creation receiver, see below)
// rewrite confidence: 0.85
// blam-cc: EAX -> command_record (the decode context)
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly. Despite the inherited name this applies the
// vehicle creation message (network action 0x1c) that vehicle_encode_network_create (0x571f20) builds: it decodes
// (0x4ec590, EAX context, ECX destination) the same 0x64-byte record -- definition, network key, owner team,
// machine key, creator key, four weapon keys (vehicle +0x2f8), five vectors (+0x52c, +0x550 forward, +0x55c up,
// +0x538, +0x544) and the +0x526 byte. Forward/up are re-orthonormalized (two cross products, both normalized),
// the creator (pooled-node table 0x687130) and machine (0x687558) keys are resolved, and the vehicle is created
// from a zeroed placement (object_new_with_datum_role_control role 1) with definition, machine object (+0x08),
// creator (+0x0c), team (+0x14), position (+0x18), forward (+0x34) and up (+0x40). The new object takes the
// network key (network_index_cache_insert_if_free 0x6870d8), the five vectors and bytes +0x525 = 1, +0x526, +0x527
// = 0, is placed (object_set_position_and_recalculate), copies velocity/angular velocity/forward/up into the
// object block (+0x68, +0x8c, +0x74, +0x80), sets +0x475, and picks up each known weapon (unit_pickup_weapon mode 0)
// or clears the slot to -1. A reliable message (**record != 0) is rejected through 0x4ec670. The previous C
// decoded into nothing and used zeroed locals for every field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;                    // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t *machine_table;                     // 0x00687558, +0x28: machine key -> index
extern uint8_t network_object_index_cache[];       // 0x006870d8

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key); // 0x4e9cd0, EAX container, ECX key, stack slot
extern void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index); // 0x4f52c0, ESI, EDI
extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack, EAX, ECX

typedef struct vehicle_network_create_message {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    int32_t weapon_keys[4];        // 0x14, vehicle +0x2f8
    real_point3d position;         // 0x24, +0x52c
    real_vector3d forward;         // 0x30, +0x550
    real_vector3d up;              // 0x3c, +0x55c
    real_vector3d velocity;        // 0x48, +0x538
    real_vector3d angular_velocity;// 0x54, +0x544
    uint8_t unknown_526;           // 0x60
    uint8_t pad_61[3];
} vehicle_network_create_message;  // 0x64 bytes

void unit_spawn_with_starting_weapons(void *command_record)
{
    vehicle_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t *keys;
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index vehicle_index;
    uint8_t *vehicle;
    int32_t i;

    if (*(int32_t *)*(int32_t **)command_record != 0) {
        message_delta_decode_compound_field_staged(command_record);
        return;
    }
    if (message_delta_decode_compound_field(command_record, &message) != 1) { // the original tests == 1
        return;
    }
    vector3d_cross_product(&side, &message.up, &message.forward);
    vector3d_cross_product(&message.up, &message.forward, &side);
    vector3d_normalize_with_length(&message.forward);
    vector3d_normalize_with_length(&message.up);
    if (message.creator_key != 0) {
        creator = ((int32_t *)object_network_id_table->handles)[message.creator_key];
    }
    if (message.machine_key != 0) {
        machine = (*(int32_t **)(machine_table + 0x28))[message.machine_key];
    }
    memset(placement, 0, sizeof(placement));
    *(datum_index *)(placement + 0x00) = message.definition;
    *(int32_t *)(placement + 0x08) = machine;
    *(int32_t *)(placement + 0x0c) = creator;
    *(int16_t *)(placement + 0x14) = message.owner_team;
    memcpy(placement + 0x18, &message.position, 12);
    memcpy(placement + 0x34, &message.forward, 12);
    memcpy(placement + 0x40, &message.up, 12);
    vehicle_index = object_new_with_datum_role_control((object_placement_data *)placement, 1);
    if (vehicle_index == (datum_index)0xffffffff) {
        return;
    }
    network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)vehicle_index);
    vehicle = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    memcpy(vehicle + 0x52c, &message.position, 12);
    memcpy(vehicle + 0x538, &message.velocity, 12);
    memcpy(vehicle + 0x544, &message.angular_velocity, 12);
    memcpy(vehicle + 0x550, &message.forward, 12);
    memcpy(vehicle + 0x55c, &message.up, 12);
    vehicle[0x526] = message.unknown_526;
    vehicle[0x525] = 1;
    vehicle[0x527] = 0;
    object_set_position_and_recalculate((real_point3d *)(vehicle + 0x52c), vehicle_index);
    memcpy(vehicle + 0x68, vehicle + 0x538, 12);
    memcpy(vehicle + 0x8c, vehicle + 0x544, 12);
    memcpy(vehicle + 0x74, vehicle + 0x550, 12);
    memcpy(vehicle + 0x80, vehicle + 0x55c, 12);
    vehicle[0x475] = 1;
    keys = (int32_t *)object_network_id_table->handles;
    for (i = 0; i < 4; i++) {
        int32_t weapon = message.weapon_keys[i] != 0 ? keys[message.weapon_keys[i]] : -1;

        if (weapon != -1) {
            unit_pickup_weapon(0, (uint32_t)weapon, vehicle_index);
        } else {
            ((int32_t *)(vehicle + 0x2f8))[i] = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x572110):

void FUN_00572110(void)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  int iVar7;
  undefined4 local_100;
  int local_f8;
  int local_f4;
  int aiStack_f0 [4];
  undefined4 local_e0;
  undefined4 local_dc;
  undefined4 local_d8;
  undefined4 local_d4;
  undefined4 local_d0;
  undefined4 local_cc;
  undefined4 local_c8;
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_bc;
  undefined4 local_b8;
  undefined4 local_b4;
  undefined4 local_b0;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined1 local_a4;
  undefined1 local_98 [12];
  undefined4 local_8c [6];
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      vector3d_cross_product(&local_d4);
      vector3d_cross_product(local_98);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      uVar6 = 0xffffffff;
      if (local_f4 != 0) {
        uVar6 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_f4 * 4);
      }
      uVar4 = 0xffffffff;
      if (local_f8 != 0) {
        uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_f8 * 4);
      }
      puVar5 = local_8c;
      for (iVar3 = 0x22; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar5 = 0;
        puVar5 = puVar5 + 1;
      }
      local_74 = local_e0;
      local_58 = local_d4;
      local_70 = local_dc;
      local_6c = local_d8;
      local_4c = local_c8;
      local_54 = local_d0;
      local_50 = local_cc;
      local_48 = local_c4;
      local_44 = local_c0;
      local_8c[2] = uVar4;
      local_8c[3] = uVar6;
      uVar2 = object_new_with_datum_role_control(local_8c,1);
      if (uVar2 != 0xffffffff) {
        FUN_004e9cd0(local_100);
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        *(undefined4 *)(iVar3 + 0x52c) = local_e0;
        *(undefined4 *)(iVar3 + 0x530) = local_dc;
        *(undefined4 *)(iVar3 + 0x534) = local_d8;
        *(undefined4 *)(iVar3 + 0x538) = local_bc;
        *(undefined4 *)(iVar3 + 0x53c) = local_b8;
        *(undefined4 *)(iVar3 + 0x540) = local_b4;
        *(undefined4 *)(iVar3 + 0x544) = local_b0;
        *(undefined4 *)(iVar3 + 0x548) = local_ac;
        *(undefined4 *)(iVar3 + 0x54c) = local_a8;
        *(undefined4 *)(iVar3 + 0x550) = local_d4;
        *(undefined4 *)(iVar3 + 0x554) = local_d0;
        *(undefined4 *)(iVar3 + 0x558) = local_cc;
        *(undefined4 *)(iVar3 + 0x55c) = local_c8;
        *(undefined4 *)(iVar3 + 0x560) = local_c4;
        *(undefined4 *)(iVar3 + 0x564) = local_c0;
        *(undefined1 *)(iVar3 + 0x526) = local_a4;
        *(undefined1 *)(iVar3 + 0x525) = 1;
        *(undefined1 *)(iVar3 + 0x527) = 0;
        object_set_position_and_recalculate();
        *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)(iVar3 + 0x538);
        *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(iVar3 + 0x53c);
        *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(iVar3 + 0x540);
        *(undefined4 *)(iVar3 + 0x8c) = *(undefined4 *)(iVar3 + 0x544);
        *(undefined4 *)(iVar3 + 0x90) = *(undefined4 *)(iVar3 + 0x548);
        *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(iVar3 + 0x54c);
        *(undefined4 *)(iVar3 + 0x74) = *(undefined4 *)(iVar3 + 0x550);
        *(undefined4 *)(iVar3 + 0x78) = *(undefined4 *)(iVar3 + 0x554);
        *(undefined4 *)(iVar3 + 0x7c) = *(undefined4 *)(iVar3 + 0x558);
        *(undefined4 *)(iVar3 + 0x80) = *(undefined4 *)(iVar3 + 0x55c);
        *(undefined4 *)(iVar3 + 0x84) = *(undefined4 *)(iVar3 + 0x560);
        *(undefined1 *)(iVar3 + 0x475) = 1;
        iVar7 = 0;
        *(undefined4 *)(iVar3 + 0x88) = *(undefined4 *)(iVar3 + 0x564);
        puVar5 = (undefined4 *)(iVar3 + 0x2f8);
        do {
          if ((aiStack_f0[iVar7] == 0) ||
             (*(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + aiStack_f0[iVar7] * 4) == -1)) {
            *puVar5 = 0xffffffff;
          }
          else {
            FUN_0056d400(0);
          }
          iVar7 = iVar7 + 1;
          puVar5 = puVar5 + 1;
        } while (iVar7 < 4);
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
