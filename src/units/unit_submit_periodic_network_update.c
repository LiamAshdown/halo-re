// unit_submit_periodic_network_update  (Ghidra: unit_submit_periodic_network_update, renamed)
// address 0x55b440, size 421 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: biped_data.network_update_sequence/network_delta_sequence/network_grenade_counts/
//   network_shield_vitality (0x527/0x528/0x52c/0x534) and unit_data.unknown_474, all matching
//   types/units.h; object.type 0xb4, body_vitality 0xe0, shield_vitality 0xe4,
//   shield_stun_ticks 0x104 and unknown_122 from types/objects.h; object_try_and_get with the
//   biped-only mask; message_delta_encode_message's shape follows the sibling file
//   src/units/unit_broadcast_state_change_event.c.
// UNSURE: message_delta_encode_message's third, fourth and fifth arguments are three
//   *consecutive slots of one stack pointer array* in the original, handed over in descending
//   slot order, and which slot means "changed mask" versus "field pointers" is not settled --
//   the array is reproduced verbatim as field_pointers[3] and the call passes the same slots.
// OBSERVATION (not relied on): the nine locals Ghidra reports at -0x1c..-0x03 are contiguous
//   and naturally aligned, and the two pointers handed to the encoder are exactly base+0x00
//   and base+0x0c, so the original source very likely declared one packed 0x19-byte
//   network-update record: int32 key, uint8 update_seq, uint8 delta_seq, uint8 is_delta,
//   int8 shield_recharging, int32 timestamp_ms, int16 grenade_counts, int16 pad, float body,
//   float shield, uint8 shield_stunned. Left as separate locals here because MSVC 7.1 packs
//   unrelated locals adjacently too, so adjacency alone does not prove the struct; recorded
//   so a hook can confirm or kill it.
// UNSURE: the original overlaps the QueryPerformanceCounter LARGE_INTEGER with field_pointers'
//   third slot (it writes local_24.s.LowPart with a pointer after the timestamp is already
//   computed). That reuse is a compiler artifact and is not reproduced.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

// large_integer (the Win32 LARGE_INTEGER union) is declared in types/math.h; src/cache and
// src/math use the same type for the same QueryPerformanceCounter/__allmul/__alldiv triple.

extern object_type_definition **object_type_definitions; // 0x0069bfdc, PTR_PTR_0069bfdc
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, QueryPerformanceFrequency() result,
                                      // owned by the timing/system module (same spelling as
                                      // src/cache and src/math use)

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *object_pooled_node_globals; // 0x00687130, the object network-id hash_table sits at +0x0c
extern int32_t message_delta_encode_message(int32_t is_delta, int32_t definition_index,
                                             void *changed, void *fields, void *types,
                                             int32_t count, char flag);         // 0x4ec940
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int32_t QueryPerformanceCounter(large_integer *counter);                  // 0x0063a0ac IAT
extern int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
extern int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);

// Builds and submits a periodic network update for the local player's biped: a full snapshot
// (param_4 == 1) carrying the resolved hash key, the two network sequence bytes, a millisecond
// timestamp, the packed grenade counts, body and shield vitality and the shield-stun flag, or a
// lighter delta that sends only the leading fields plus the biped's own 0x52c network block.
// Advances the per-biped delta sequence counter on success (wrapping at -1) and clears the
// pending-resend byte either way.
int32_t unit_submit_periodic_network_update(int32_t hash_key, uint32_t param_2, uint32_t param_3,
                                             int32_t param_4)
{
    // object_try_and_get takes the object index in ECX and the type mask on the stack
    // (src/objects spells it that way and its disassembly note confirms it), so Ghidra's
    // `object_try_and_get(1)` is mask = 1 (biped) with the index left as an unresolved register
    // read. None of this function's four parameters is a plausible object index, so the index is
    // almost certainly the local player's controlled unit, fetched into ECX by the caller.
    // UNSURE: the ECX source.
    datum_index target_index = k_datum_index_none; // UNSURE: unresolved ECX read
    object *obj = object_try_and_get(target_index, 1);
    if (obj == 0) {
        return 0;
    }

    {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

        int32_t resolved_key = 0;               // local_1c
        uint8_t update_sequence;                // local_18
        uint8_t delta_sequence;                 // local_17
        uint8_t is_delta;                       // local_16
        char shield_recharging;                 // local_15, a signed char in the original
        int32_t timestamp_milliseconds;         // local_14
        int16_t packed_grenade_counts;          // local_10[0]
        int32_t body_vitality_bits;             // local_c
        float shield_fraction;                  // local_8
        uint8_t shield_stunned;                 // local_4

        void *field_pointers[3];                // local_2c, local_28, local_24.s.LowPart
        large_integer counter;
        int32_t definition_index;
        int32_t result;

        if (hash_key != -1) {
            resolved_key = hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), hash_key);
            if (resolved_key == -1) {
                resolved_key = 0;
            }
        }

        update_sequence = biped->network_update_sequence;
        delta_sequence = biped->network_delta_sequence;
        shield_recharging = (char)obj->unknown_122;
        is_delta = (uint8_t)(param_4 == 0);

        QueryPerformanceCounter(&counter);
        timestamp_milliseconds = __alldiv(__allmul(counter.parts.low_part, counter.parts.high_part,
                                                   1000, 0),
                                          (int32_t)performance_frequency,
                                          (int32_t)(performance_frequency >> 32));

        definition_index = (int32_t)object_type_definitions[obj->type]->network_delta_message_type;
        obj->unknown_122 = 0;

        if (param_4 == 1) {
            if (shield_recharging == 1) {
                shield_fraction = obj->shield_vitality * 0.33333334f;
            } else {
                shield_fraction = biped->network_shield_vitality;
            }
            body_vitality_bits = *(int32_t *)&obj->body_vitality;
            packed_grenade_counts = *(int16_t *)&unit->grenade_counts[0]; // object 0x31e as one int16
            shield_stunned = (uint8_t)(obj->shield_stun_ticks > 0);

            field_pointers[0] = &biped->network_grenade_counts;
            field_pointers[1] = &packed_grenade_counts;
            field_pointers[2] = &resolved_key;
            result = message_delta_encode_message(1, definition_index, &field_pointers[2],
                                                   &field_pointers[1], &field_pointers[0], 1, 0);
        } else {
            field_pointers[1] = &resolved_key;
            field_pointers[2] = &biped->network_grenade_counts;
            result = message_delta_encode_message(0, definition_index, &field_pointers[1],
                                                   &field_pointers[2], 0, 1, 0);
        }

        if (result > 0) {
            // re-read rather than reuse delta_sequence: the encoder runs in between, and the
            // original loads 0x528 fresh here. The wrap test is on a signed char against -1.
            char next = (char)(biped->network_delta_sequence + 1);
            biped->network_delta_sequence = (uint8_t)next;
            if (next == -1) {
                biped->network_delta_sequence = 0;
            }
        }
        unit->unknown_474 = 0;
        (void)update_sequence;
        (void)is_delta;
        (void)timestamp_milliseconds;
        (void)body_vitality_bits;
        (void)shield_fraction;
        (void)shield_stunned;
        (void)delta_sequence;
        return result;
    }
}

#if 0
Original Ghidra decompilation (0x55b440):

int FUN_0055b440(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  char cVar3;
  undefined8 uVar4;
  int flag;
  LARGE_INTEGER *changed_offset;
  LARGE_INTEGER *items;
  int **type_offset;
  int *local_2c;
  int *local_28;
  LARGE_INTEGER local_24;
  int local_1c;
  undefined1 local_18;
  undefined1 local_17;
  undefined1 local_16;
  char local_15;
  undefined4 local_14;
  undefined2 local_10 [2];
  undefined4 local_c;
  float local_8;
  undefined1 local_4;

  iVar1 = object_try_and_get(1);
  if (iVar1 != 0) {
    local_1c = 0;
    if (param_1 != -1) {
      local_1c = hash_table_get();
      if (local_1c == -1) {
        local_1c = 0;
      }
    }
    local_17 = *(undefined1 *)(iVar1 + 0x528);
    local_18 = *(undefined1 *)(iVar1 + 0x527);
    local_15 = *(char *)(iVar1 + 0x122);
    local_16 = param_4 == 0;
    QueryPerformanceCounter(&local_24);
    uVar4 = __allmul(local_24.s.LowPart,local_24.s.HighPart,1000,0);
    local_14 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
    iVar2 = *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10);
    *(undefined1 *)(iVar1 + 0x122) = 0;
    if (param_4 == 1) {
      if (local_15 == '\x01') {
        local_8 = *(float *)(iVar1 + 0xe4) * 0.33333334;
      }
      else {
        local_8 = *(float *)(iVar1 + 0x534);
      }
      local_c = *(undefined4 *)(iVar1 + 0xe0);
      local_10[0] = *(undefined2 *)(iVar1 + 0x31e);
      local_4 = 0 < *(short *)(iVar1 + 0x104);
      local_28 = (int *)local_10;
      type_offset = &local_2c;
      items = (LARGE_INTEGER *)&local_28;
      changed_offset = &local_24;
      flag = 1;
      local_2c = (int *)(iVar1 + 0x52c);
      local_24.s.LowPart = (DWORD)&local_1c;
    }
    else {
      type_offset = (int **)0x0;
      local_28 = &local_1c;
      items = &local_24;
      changed_offset = (LARGE_INTEGER *)&local_28;
      flag = 0;
      local_24.s.LowPart = (DWORD)(int *)(iVar1 + 0x52c);
    }
    iVar2 = message_delta_encode_message
                      (flag,iVar2,(int)changed_offset,(void **)items,(int)type_offset,1,'\0');
    if ((0 < iVar2) &&
       (cVar3 = *(char *)(iVar1 + 0x528) + '\x01', *(char *)(iVar1 + 0x528) = cVar3, cVar3 == -1)) {
      *(undefined1 *)(iVar1 + 0x528) = 0;
    }
    *(undefined1 *)(iVar1 + 0x474) = 0;
    return iVar2;
  }
  return 0;
}
#endif
