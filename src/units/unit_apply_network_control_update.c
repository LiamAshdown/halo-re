// unit_apply_network_control_update  (Ghidra: unit_apply_network_control_update)
// address 0x566c90, size 322 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.1
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *network_message_table; // 0x00687130, PTR_DAT_00687130; +0x28 is a pointer
                                       // to the per-player unit handle array

extern uint8_t message_delta_decode_compound_field(void);                          // 0x4ec590, UNSURE: no traced args
extern void message_delta_decode_compound_field_staged(void);                              // 0x4ec670, UNSURE: no traced args
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void *datum_get();                                    // 0x4d0680, src/memory/datum_get.c:
  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
// void *datum_get(datum_index handle, data_array *array), handle in EAX and array in ECX.
// Ghidra bound neither operand at the two call sites below, so the declaration is unprototyped.
// real signature (unit_update_stance_and_jump.c): void unit_update_stance_and_jump(uint32_t
//   unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check,
//   uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t
//   weapon_class_index, int32_t fire_trigger_event, uint8_t require_still);
// Ghidra recovered 9 bit-packed sub-windows here, not the 10 typed parameters; see the header.
extern void unit_update_stance_and_jump(uint32_t unit_index, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                          uint32_t e, uint32_t f, uint32_t g, uint32_t h);   // 0x566de0, UNSURE signature
extern void unit_release_transient_state_and_detach(uint32_t unit_index, uint8_t is_light_reset);              // 0x568cb0, UNSURE signature
extern void network_index_cache_remove(void);                              // 0x4e9d40, UNSURE: no traced args

// unit_network_control_packet is declared in types/units.h.

void unit_apply_network_control_update(unit_network_control_packet *packet) // blam-cc: in_EAX -> packet
{
    if (*packet->kind_ptr != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    if (message_delta_decode_compound_field() == 0) {
        return;
    }

    int32_t index = *(int32_t *)(packet->payload + 0x00); // UNSURE: local_24
    if (index == 0) {
        return;
    }
    // PTR_DAT_00687130 + 0x28 is itself a pointer; the original indexes *through* it
    uint32_t unit_handle = *(uint32_t *)(*(uint8_t **)(network_message_table + 0x28) + index * 4);
    if (unit_handle == (uint32_t)-1) {
        return;
    }

    object *unit_obj = object_try_and_get((datum_index)3, 3); // UNSURE: literal `3` reproduced as both args, see original
    if (unit_obj != 0) {
        unit_obj->vitality_flags = unit_obj->vitality_flags | _object_health_frozen_bit;
        unit_obj->body_vitality = 0.0f;
        unit_obj->shield_vitality = 0.0f;
    }

    if (packet->payload[0x0c] == 1) { // UNSURE: local_20
        unit_update_stance_and_jump(unit_handle,
                     *(uint32_t *)(packet->payload + 0x00), // UNSURE: overlapping-window reconstruction, see file header
                     *(uint32_t *)(packet->payload + 0x01),
                     *(uint32_t *)(packet->payload + 0x02),
                     *(uint32_t *)(packet->payload + 0x03),
                     *(uint32_t *)(packet->payload + 0x04),
                     *(uint32_t *)(packet->payload + 0x18), // UNSURE: local_14
                     *(uint32_t *)(packet->payload + 0x1c), // UNSURE: uStack_18
                     (uint32_t)(-(int32_t)(packet->payload[0x1a] != 1)) & *(uint32_t *)(packet->payload + 0x20)); // UNSURE
    }

    unit_obj = object_try_and_get((datum_index)3, 3);
    if (unit_obj != 0) {
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        if (unit->controlling_player != (datum_index)-1) {
            void *d = datum_get(); // UNSURE: both operands are register-carried
            if (d != 0) {
                *(uint32_t *)((uint8_t *)d + 0x2c) = *(uint32_t *)(packet->payload + 0x24); // UNSURE: local_8
            }
        }
    }

    unit_release_transient_state_and_detach(unit_handle, *(uint32_t *)(packet->payload + 0x03)); // UNSURE

    unit_obj = object_try_and_get((datum_index)3, 3);
    if (unit_obj != 0) {
        unit_obj->network_role = 3;
    }

    object_header *header = &((object_header *)object_data->data)[unit_handle & 0xffff];
    if ((header->flags & 8) == 0) {
        network_index_cache_remove();
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
