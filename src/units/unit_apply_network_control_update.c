// unit_apply_network_control_update  (Ghidra: unit_apply_network_control_update)
// address 0x566c90, size 322 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.85 (REWRITTEN below; earlier: 0.1)
// evidence: types/objects.h object.vitality_flags (0x106, _object_health_frozen_bit),
//   .body_vitality/.shield_vitality (0xe0/0xe4), .network_role (0x004),
//   unit_data.controlling_player (0x218); object_try_and_get (type mask 3 = unit).
// register convention: an incoming packet/record pointer in EAX.
//   // blam-cc: in_EAX -> packet
// UNSURE: this function is almost entirely unrecoverable from the decompilation. The bulk of
//   its body reconstructs five call arguments to unit_update_stance_and_jump (outside this batch's address
//   range) via heavily overlapping `CONCAT13(x,CONCAT12(y,CONCAT11(z,w)))` byte windows shifted
//   one byte apart -- a pattern typical of unpacking bit-packed (not byte-aligned) fields out of
//   a small local buffer, most likely filled earlier by bit_stream reads this pack does not
//   include. Reconstructing the real field boundaries would require the network message tag
//   definition for whatever packet type this is, which is outside this batch's evidence.
//   Rather than invent plausible-looking field values, the packet is passed through as an
//   opaque buffer and the five unit_update_stance_and_jump arguments are left as raw sub-windows of it,
//   clearly marked. The dispatch structure (the outer `if (*(int *)*packet == 0)`, the three
//   object_try_and_get/datum_get side effects, and the trailing network_index_cache_remove call) are
//   reproduced faithfully; the payload interpretation is not.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x566c90..0x566dd0): the message decodes
// (0x4ec590, EAX context, ECX destination) into a 0x24-byte record (below). The unit (pooled-node key +0x00) has its
// health frozen (+0x106 |= 4) and body/shield vitality zeroed; when +0x04 is 1 the ten-argument
// unit_update_stance_and_jump (0x566de0) runs with the bytes +0x05..+0x09, the angle +0x10, the class +0x0c, the
// 2-D vector at +0x14 (NULL when +0x0a is 1) and 1; the controlling player (+0x218, players 0x87a480) takes +0x1c
// at +0x2c; unit_release_transient_state_and_detach(unit, +0x06); the object's +0x04 becomes 3; and unless the
// header's bit 8 is set the unit leaves the network index cache (0x6870d8). The previous C decoded into nothing,
// looked up object index 3 instead of the unit, and passed overlapping byte windows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;                    // 0x008603b0
extern uint8_t *object_network_id_table;             // 0x00687130, +0x28: network key -> object index
extern data_array *player_data;                    // 0x0087a480
extern uint8_t network_object_index_cache[];       // 0x006870d8

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX handle, ESI array
extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
    uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle,
    int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still); // 0x566de0, cdecl
extern void unit_release_transient_state_and_detach(uint32_t unit_index, uint8_t is_light_reset); // 0x568cb0
extern uint8_t network_index_cache_remove(uint8_t *container, int32_t key); // 0x4e9d40, EAX container, ESI key

typedef struct unit_network_control_message {
    int32_t unit_key;              // 0x00
    uint8_t update_stance;         // 0x04, 1: call unit_update_stance_and_jump
    uint8_t stance_flags[5];       // 0x05..0x09, its five byte arguments (+0x06 also the detach flag)
    uint8_t no_throttle;           // 0x0a, 1: pass no vector
    uint8_t pad_0b;
    int16_t weapon_class_index;    // 0x0c
    int16_t pad_0e;
    float turn_angle;              // 0x10
    real_vector2d throttle;        // 0x14
    uint32_t player_2c;            // 0x1c -> controlling player +0x2c
    uint32_t pad_20;
} unit_network_control_message;    // 0x24 bytes

void unit_apply_network_control_update(unit_network_control_packet *packet) // blam-cc: EAX -> packet (decode context)
{
    unit_network_control_message message;
    const real_vector2d *throttle;
    uint32_t unit_index;
    uint8_t *unit;

    if (*packet->kind_ptr != 0) {
        message_delta_decode_compound_field_staged(packet);
        return;
    }
    if (message_delta_decode_compound_field(packet, &message) == 0 || message.unit_key == 0) {
        return;
    }
    unit_index = (uint32_t)(*(int32_t **)(object_network_id_table + 0x28))[message.unit_key];
    if (unit_index == 0xffffffff) {
        return;
    }
    throttle = message.no_throttle == 1 ? (const real_vector2d *)0 : &message.throttle;
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit != 0) {
        unit[0x106] |= 4;
        *(real *)(unit + 0xe0) = 0.0f;
        *(real *)(unit + 0xe4) = 0.0f;
    }
    if (message.update_stance == 1) {
        unit_update_stance_and_jump(unit_index, message.stance_flags[0], message.stance_flags[1],
            message.stance_flags[2], message.stance_flags[3], message.stance_flags[4], message.turn_angle,
            message.weapon_class_index, throttle, 1);
    }
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit != 0 && *(datum_index *)(unit + 0x218) != (datum_index)0xffffffff) {
        uint8_t *player = (uint8_t *)datum_get(*(datum_index *)(unit + 0x218), player_data);

        if (player != 0) {
            *(uint32_t *)(player + 0x2c) = message.player_2c;
        }
    }
    unit_release_transient_state_and_detach(unit_index, message.stance_flags[1]);
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit != 0) {
        *(int32_t *)(unit + 0x4) = 3;
    }
    if ((((object_header *)object_data->data)[unit_index & 0xffff].flags & 8) == 0) {
        network_index_cache_remove(network_object_index_cache, (int32_t)unit_index);
    }
}

#if 0
Original Ghidra decompilation (0x566c90):

void FUN_00566c90(void)

{
  uint uVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int local_24;
  char local_20;
  undefined1 local_1f;
  undefined1 uStack_1e;
  undefined1 uStack_1d;
  undefined1 uStack_1c;
  undefined1 uStack_1b;
  char cStack_1a;
  undefined1 uStack_19;
  undefined4 uStack_18;
  undefined4 local_14;
  undefined1 local_10 [8];
  undefined4 local_8;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (((cVar2 != '\0') && (local_24 != 0)) &&
       (uVar1 = *(uint *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_24 * 4), uVar1 != 0xffffffff)) {
      iVar3 = object_try_and_get(3);
      if (iVar3 != 0) {
        *(byte *)(iVar3 + 0x106) = *(byte *)(iVar3 + 0x106) | 4;
        *(undefined4 *)(iVar3 + 0xe0) = 0;
        *(undefined4 *)(iVar3 + 0xe4) = 0;
      }
      if (local_20 == '\x01') {
        FUN_00566de0(uVar1,CONCAT13(uStack_1c,CONCAT12(uStack_1d,CONCAT11(uStack_1e,local_1f))),
                     CONCAT13(uStack_1b,CONCAT12(uStack_1c,CONCAT11(uStack_1d,uStack_1e))),
                     CONCAT13(cStack_1a,CONCAT12(uStack_1b,CONCAT11(uStack_1c,uStack_1d))),
                     CONCAT13(uStack_19,CONCAT12(cStack_1a,CONCAT11(uStack_1b,uStack_1c))),
                     CONCAT13((undefined1)uStack_18,
                              CONCAT12(uStack_19,CONCAT11(cStack_1a,uStack_1b))),local_14,uStack_18,
                     -(uint)(cStack_1a != '\x01') & (uint)local_10,1);
      }
      iVar3 = object_try_and_get(3);
      if ((iVar3 != 0) && (*(int *)(iVar3 + 0x218) != -1)) {
        iVar3 = datum_get();
        if (iVar3 != 0) {
          *(undefined4 *)(iVar3 + 0x2c) = local_8;
        }
      }
      FUN_00568cb0(uVar1,CONCAT13(uStack_1b,CONCAT12(uStack_1c,CONCAT11(uStack_1d,uStack_1e))));
      iVar3 = object_try_and_get(3);
      if (iVar3 != 0) {
        *(undefined4 *)(iVar3 + 4) = 3;
      }
      if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (uVar1 & 0xffff) * 0xc) & 8) == 0) {
        FUN_004e9d40();
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
