// unit_network_create_update_apply  (Ghidra: unit_network_create_update_apply, renamed)
// address 0x55b110, size 702 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (see REWRITTEN; the UNSURE note below is superseded)
// evidence: the writes into the newly created object at the end all match documented
//   biped_data fields exactly by offset (network_body_vitality 0x530, network_shield_vitality
//   0x534, network_shield_stunned 0x538, network_grenade_counts 0x52c, network_update_sequence
//   0x527, saved_control.zoom_level-derived 0x480, network_baseline_valid-adjacent 0x475,
//   unknown_526/0x528) and unit_data (control_flags bit 0x80000 0x204, desired_grenade_index
//   0x31e, saved_control.looking_vector 0x4ac). object.body_vitality (0xe0) and shield_vitality
//   (0xe4) also match objects.h.
// UNSURE: the whole first half of the function -- everything read out of local_10c..local_44
//   before the object is created -- is populated by vector3d_cross_product /
//   vector3d_normalize_with_length calls whose arguments Ghidra could not bind, and by direct
//   reads of the incoming network record (*in_EAX) that Ghidra also lost. This rewrite treats
//   that half as an opaque "decode the incoming creation record into an object_placement_data"
//   step and does not claim to know its exact field layout; it is preserved as a raw byte
//   buffer matching Ghidra's own stack allocation size.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x55b110..0x55b3c8): the biped creation
// receiver (network action 0x1d), the counterpart of the vehicle one (0x572110). The message decodes (0x4ec590, EAX
// context, ECX destination; the original tests == 1) into a 0x8c-byte record: definition, network key, owner
// team, machine and creator keys, position, forward, up, a vector for placement +0x28, a 0x30-byte block for
// placement +0x58, then the unit flag 0x80000 (+0x204), the +0x344 scalar, the update sequence (+0x527), the grenade
// counts (+0x52c, an unaligned word at record +0x7d), body vitality (+0x530), shield vitality (+0x534) and the
// shield-stunned byte (+0x538). Forward/up are re-orthonormalized, the keys resolved (0x687130 / 0x687558), the
// biped created from the placement (role 1) and given the network key, and the network block and the live fields
// derived from it are written. The previous C decoded into nothing and wrote zeroed locals.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;                    // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
extern network_id_table *machine_table;
extern uint8_t network_object_index_cache[];       // 0x006870d8

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key); // 0x4e9cd0, EAX container, ECX key, stack slot

typedef struct biped_network_create_message {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    real_point3d position;         // 0x14 -> placement +0x18
    real_vector3d forward;         // 0x20 -> placement +0x34
    real_vector3d up;              // 0x2c -> placement +0x40
    real_vector3d vector_38;       // 0x38 -> placement +0x28
    uint8_t block_44[0x30];        // 0x44 -> placement +0x58
    uint8_t flag_80000;            // 0x74 -> unit flags (+0x204) bit 0x80000
    uint8_t pad_75[3];
    uint32_t scalar_344;           // 0x78 -> +0x344
    uint8_t update_sequence;       // 0x7c -> +0x527
    uint8_t grenade_counts[2];     // 0x7d -> +0x52c (unaligned int16)
    uint8_t pad_7f;
    uint32_t body_vitality;        // 0x80 -> +0x530
    real shield_vitality;          // 0x84 -> +0x534
    uint8_t shield_stunned;        // 0x88 -> +0x538
    uint8_t pad_89[3];
} biped_network_create_message;    // 0x8c bytes

void unit_network_create_update_apply(void *incoming_record)
{
    biped_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index biped_index;
    uint8_t *biped;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged(incoming_record);
        return;
    }
    if (message_delta_decode_compound_field(incoming_record, &message) != 1) { // the original tests == 1
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
        machine = (*(int32_t **)&machine_table->handles)[message.machine_key];
    }
    memset(placement, 0, sizeof(placement));
    *(datum_index *)(placement + 0x00) = message.definition;
    *(int32_t *)(placement + 0x08) = machine;
    *(int32_t *)(placement + 0x0c) = creator;
    *(int16_t *)(placement + 0x14) = message.owner_team;
    memcpy(placement + 0x18, &message.position, 12);
    memcpy(placement + 0x28, &message.vector_38, 12);
    memcpy(placement + 0x34, &message.forward, 12);
    memcpy(placement + 0x40, &message.up, 12);
    memcpy(placement + 0x58, message.block_44, 0x30);
    biped_index = object_new_with_datum_role_control((object_placement_data *)placement, 1);
    if (biped_index == (datum_index)0xffffffff) {
        return;
    }
    network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)biped_index);
    biped = (uint8_t *)((object_header *)object_data->data)[biped_index & 0xffff].data;
    *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality = message.body_vitality;
    ((biped_object *)biped)->biped.network_shield_vitality = message.shield_vitality;
    biped[0x538] = message.shield_stunned;
    memcpy(biped + 0x52c, message.grenade_counts, 2);
    ((unit_object *)biped)->base.shield_vitality = ((biped_object *)biped)->biped.network_shield_vitality * 3.0f;   // shield vitality (0x672c3c = 3.0)
    biped[0x527] = message.update_sequence;
    *(uint32_t *)&((unit_object *)biped)->base.body_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality;  // body vitality
    ((unit_object *)biped)->unit.saved_control.zoom_level = (int16_t)((unit_object *)biped)->unit.desired_zoom_level;
    biped[0x526] = 1;
    biped[0x528] = 0;
    biped[0x475] = 1;
    ((unit_object *)biped)->base.shield_stun_ticks = biped[0x538] == 1;
    memcpy(biped + 0x4ac, biped + 0x494, 12);
    *(int16_t *)(biped + 0x31e) = ((biped_object *)biped)->biped.network_grenade_counts;
    if (message.flag_80000 != 0) {
        ((unit_object *)biped)->unit.flags |= 0x80000;
    } else {
        ((unit_object *)biped)->unit.flags &= ~0x80000u;
    }
    *(uint32_t *)(biped + 0x344) = message.scalar_344;
}

#if 0
Original Ghidra decompilation (0x55b110):

void FUN_0055b110(void)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_128;
  int local_120;
  int local_11c;
  undefined4 local_10c;
  undefined4 local_108;
  undefined4 local_104;
  undefined4 local_100;
  undefined4 local_fc;
  undefined4 local_f8;
  undefined4 local_f4;
  undefined4 local_f0;
  undefined4 local_ec;
  undefined4 local_e8 [12];
  char local_b8;
  undefined4 local_b4;
  undefined1 local_b0;
  undefined2 local_af;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined1 local_a4;
  undefined1 local_98 [12];
  undefined4 local_8c [10];
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_34 [13];

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      vector3d_cross_product(&local_10c);
      vector3d_cross_product(local_98);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      uVar5 = 0xffffffff;
      if (local_11c != 0) {
        uVar5 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_11c * 4);
      }
      uVar4 = 0xffffffff;
      if (local_120 != 0) {
        uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_120 * 4);
      }
      puVar6 = local_8c;
      for (iVar3 = 0x22; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      local_8c[2] = uVar4;
      local_64 = local_f4;
      local_5c = local_ec;
      local_60 = local_f0;
      local_58 = local_10c;
      local_50 = local_104;
      local_54 = local_108;
      local_4c = local_100;
      local_8c[3] = uVar5;
      local_44 = local_f8;
      local_48 = local_fc;
      puVar6 = local_e8;
      puVar7 = local_34;
      for (iVar3 = 0xc; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar7 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
      }
      uVar2 = object_new_with_datum_role_control(local_8c,1);
      if (uVar2 != 0xffffffff) {
        FUN_004e9cd0(local_128);
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        *(undefined4 *)(iVar3 + 0x530) = local_ac;
        *(undefined4 *)(iVar3 + 0x534) = local_a8;
        *(undefined1 *)(iVar3 + 0x538) = local_a4;
        *(undefined2 *)(iVar3 + 0x52c) = local_af;
        *(float *)(iVar3 + 0xe4) = *(float *)(iVar3 + 0x534) * 3.0;
        *(undefined1 *)(iVar3 + 0x527) = local_b0;
        *(undefined4 *)(iVar3 + 0xe0) = *(undefined4 *)(iVar3 + 0x530);
        *(short *)(iVar3 + 0x480) = (short)*(char *)(iVar3 + 0x321);
        *(undefined1 *)(iVar3 + 0x526) = 1;
        *(undefined1 *)(iVar3 + 0x528) = 0;
        *(undefined1 *)(iVar3 + 0x475) = 1;
        *(ushort *)(iVar3 + 0x104) = (ushort)(*(char *)(iVar3 + 0x538) == '\x01');
        *(undefined4 *)(iVar3 + 0x4ac) = *(undefined4 *)(iVar3 + 0x494);
        *(undefined4 *)(iVar3 + 0x4b0) = *(undefined4 *)(iVar3 + 0x498);
        *(undefined4 *)(iVar3 + 0x4b4) = *(undefined4 *)(iVar3 + 0x49c);
        *(undefined2 *)(iVar3 + 0x31e) = *(undefined2 *)(iVar3 + 0x52c);
        if (local_b8 == '\0') {
          uVar2 = *(uint *)(iVar3 + 0x204) & 0xfff7ffff;
        }
        else {
          uVar2 = *(uint *)(iVar3 + 0x204) | 0x80000;
        }
        *(uint *)(iVar3 + 0x204) = uVar2;
        *(undefined4 *)(iVar3 + 0x344) = local_b4;
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
