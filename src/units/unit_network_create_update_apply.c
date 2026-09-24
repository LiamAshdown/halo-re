// unit_network_create_update_apply  (Ghidra: unit_network_create_update_apply, renamed)
// address 0x55b110, size 702 bytes
// name confidence: 0.35   rewrite confidence: 0.2
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern real vector3d_normalize_with_length(real_vector3d *v);                                    // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
// CORRECTIONS made in the phase-4 review pass (each was a field-identity error, not a style
// change):
//   - object 0x31e is unit_data.grenade_counts written as one int16, not the
//     0x31d desired_grenade_index byte;
//   - object 0x204 is unit_data.flags, not unit_data.control_flags (0x208);
//   - 0x494 -> 0x4ac is saved_control.facing_vector -> saved_control.looking_vector, both inside
//     the cached 0x478 control record, not unit_data.desired_looking_vector (0x254);
//   - the four record -> biped writes (0x52c/0x530/0x534/0x538) and the 0x344 write were dropped
//     by the earlier rewrite and are restored below as explicit assignments from named decoded
//     values, so the set of fields this function writes is complete even where the source of a
//     value is still unknown.

extern uint8_t message_delta_decode_compound_field(void); // 0x4ec590, UNSURE module: tests whether this network role should apply the create
extern void message_delta_decode_compound_field_staged(void); // 0x4ec670, UNSURE module: rejects/discards the incoming record
extern void network_index_cache_insert_if_free(uint32_t param_1); // 0x4e9cd0, UNSURE module
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0

// Handles an incoming network unit-creation update: validates it (message_delta_decode_compound_field), decodes the
// incoming record into a 0x88-byte object placement buffer (UNSURE, see file header), spawns the
// object via object_new_with_datum_role_control, and then initialises the new unit's network
// block, live vitality, grenade counts, zoom and cached control record from the record.
void unit_network_create_update_apply(void *incoming_record)
{
    // UNSURE: *(int*)*incoming_record == 0 selects "apply the create", matching message_delta_decode_compound_field's
    // gate; nonzero takes the reject path (message_delta_decode_compound_field_staged).
    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    if (message_delta_decode_compound_field() != 1) {  // the original tests == '\x01', not merely non-zero
        return;
    }

    {
        // Decoded out of the incoming record by the (unrecovered) prologue: Ghidra shows the
        // cross-product / normalise calls building an orientation into local_10c..local_ec and
        // a second block into local_98, then two message-table lookups into local_11c/local_120.
        // All of it lands inside the placement buffer below, so the buffer is carried opaquely.
        // UNSURE: every one of these, including which record field each comes from.
        uint32_t decoded_body_vitality_bits = 0;   // local_ac -> biped 0x530
        float    decoded_shield_vitality = 0.0f;   // local_a8 -> biped 0x534
        int8_t   decoded_shield_stunned = 0;       // local_a4 -> biped 0x538
        int16_t  decoded_grenade_counts = 0;       // local_af -> biped 0x52c
        uint8_t  decoded_update_sequence = 0;      // local_b0 -> biped 0x527
        float    decoded_scalar_344 = 0.0f;        // local_b4 -> unit  0x344
        char     decoded_flag_80000 = 0;           // local_b8 -> unit  0x204 bit 0x80000
        uint32_t decoded_notify_argument = 0;      // local_128 -> network_index_cache_insert_if_free

        // Ghidra zeroes 0x22 dwords (0x88 bytes) through an undefined4 * cursor, then copies
        // 0xc dwords (0x30 bytes) from the decoded orientation block into the buffer's tail at
        // +0x58. Both strides are 4 bytes; the byte count is what matters here.
        uint8_t placement[0x88];
        uint32_t new_object_index;
        {
            int32_t i;
            for (i = 0; i < 0x88; i++) {
                placement[i] = 0;
            }
        }

        new_object_index = object_new_with_datum_role_control((object_placement_data *)placement, 1);
        if (new_object_index == (uint32_t)k_datum_index_none) {
            return;
        }

        network_index_cache_insert_if_free(decoded_notify_argument);

        {
            object *obj = ((object_header *)object_data->data)[new_object_index & 0xffff].data;
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

            // the record -> biped network block, in the original's order
            *(uint32_t *)&biped->network_body_vitality = decoded_body_vitality_bits;
            biped->network_shield_vitality = decoded_shield_vitality;
            biped->network_shield_stunned = decoded_shield_stunned;
            biped->network_grenade_counts = decoded_grenade_counts;

            // then the live object fields, read back out of the block just written
            obj->shield_vitality = biped->network_shield_vitality * 3.0f;
            biped->network_update_sequence = decoded_update_sequence;
            *(uint32_t *)&obj->body_vitality = *(uint32_t *)&biped->network_body_vitality;

            // int8_t -> int16_t, sign-extended, into the cached control record
            unit->saved_control.zoom_level = (int16_t)unit->desired_zoom_level;
            biped->unknown_526 = 1;
            biped->network_delta_sequence = 0;
            unit->unknown_475 = 1;
            obj->shield_stun_ticks = (int16_t)(uint16_t)(biped->network_shield_stunned == 1);

            // inside the cached 0x478 control record: facing_vector (0x494) -> looking_vector (0x4ac)
            unit->saved_control.looking_vector = unit->saved_control.facing_vector;

            // object 0x31e is unit_data.grenade_counts as one int16
            *(int16_t *)&unit->grenade_counts[0] = biped->network_grenade_counts;

            if (decoded_flag_80000 == 0) {
                unit->flags = unit->flags & ~0x80000u;   // object 0x204 = unit_data.flags
            } else {
                unit->flags = unit->flags | 0x80000u;
            }
            unit->unknown_344 = decoded_scalar_344;
        }
    }
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
