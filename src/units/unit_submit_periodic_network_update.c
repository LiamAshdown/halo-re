// unit_submit_periodic_network_update  (Ghidra: unit_submit_periodic_network_update, renamed)
// address 0x55b440, size 421 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// REWRITTEN from objdump 0x55b440..0x55b5e5 (the draft treated the object index as an unresolved ECX read and dropped the
//   EAX/EDX arguments of the encoder). Stack: (object_index, buffer, bit_budget, update_type); this is the biped column
//   +0x6c (override_call_6c) of object_type_definition, the sibling of vehicle_encode_network_update (0x5724d0) and it is
//   called by object_type_override_call_0x6c with the same (index, buffer 0x871de0, budget 0x7ff8, full flag).
//   The header record {key, update seq (0x527), delta seq (0x528), is_delta, the object's shield_update_pending byte
//   (0x122), timestamp ms} is followed, for a full update (update_type == 1), by the baseline record
//   {grenade counts (object 0x31e as an int16), body vitality (0xe0), shield (0xe4 / 3 while the shield is recharging,
//   else the biped's 0x534), stunned (shield_stun_ticks > 0)}; both are handed to message_delta_encode_message
//   (EAX = buffer, EDX = bit budget, three slots of one pointer array) together with the biped's own delta record at 0x52c.
//   A delta update sends the header and the 0x52c record only. The pending byte is cleared before the encoder runs and
//   the delta sequence advances (0xff wraps to 0) when it wrote something; the unit's forced-update byte (0x474) is
//   cleared either way. Returns the encoder's bit count, or 0 without a biped.
// VERIFIED against disassembly 0x55b440..0x55b5e5 (2026-09-30)
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)
// blam-cc: stack -> (object_index, buffer, bit_budget, update_type)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct biped_network_update_header {
    int32_t network_key;            // +0x00 hash_table_get result, 0 when unknown
    uint8_t update_sequence;        // +0x04 biped_data.network_update_sequence (0x527)
    uint8_t delta_sequence;         // +0x05 biped_data.network_delta_sequence (0x528)
    uint8_t is_delta;               // +0x06 update_type == 0
    uint8_t shield_update_pending;  // +0x07 object.shield_update_pending (0x122) before it is cleared
    int32_t timestamp_milliseconds; // +0x08
} biped_network_update_header;      // size 0x0c

typedef struct biped_network_update_baseline {
    int16_t grenade_counts;         // +0x00 object 0x31e (both counts)
    int16_t pad_02;
    float body_vitality;            // +0x04 object 0xe0
    float shield_vitality;          // +0x08 object 0xe4 * 1/3 while recharging, else biped_data 0x534
    uint8_t shield_stunned;         // +0x0c shield_stun_ticks > 0
} biped_network_update_baseline;

extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc, an ARRAY (was a pointer variable)
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, QueryPerformanceFrequency() result

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern network_id_table *object_network_id_table; // 0x00687130
extern int32_t message_delta_encode_message(void *buffer, int32_t bit_budget, int32_t flag,
    int32_t message_type, void *changed, void *items, void *types, int32_t count,
    char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX bit_budget, rest on the stack
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
extern int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);

int32_t unit_submit_periodic_network_update(datum_index object_index, void *buffer, int32_t bit_budget,
                                             int32_t update_type)
{
    object *obj = object_try_and_get(object_index, 1);
    unit_data *unit;
    biped_data *biped;
    biped_network_update_header header;
    biped_network_update_baseline baseline;
    void *slots[3];                 // esp+0x18, +0x1c, +0x20 (the last shares the timestamp counter's slot)
    large_integer counter;
    int32_t message_type;
    int32_t key = 0;
    int32_t result;

    if (obj == 0) {
        return 0;
    }
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if (object_index != (datum_index)0xffffffff) {
        key = hash_table_get(&object_network_id_table->id_to_index, (int32_t)object_index);
        if (key == -1) {
            key = 0;
        }
    }
    header.network_key = key;
    header.update_sequence = biped->network_update_sequence;
    header.delta_sequence = biped->network_delta_sequence;
    header.is_delta = (uint8_t)(update_type == 0);
    header.shield_update_pending = obj->shield_update_pending;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    header.timestamp_milliseconds = __alldiv(__allmul(counter.parts.low_part, counter.parts.high_part, 1000, 0),
                                             (int32_t)performance_frequency,
                                             (int32_t)(performance_frequency >> 32));

    message_type = (int32_t)object_type_definitions[obj->type]->network_delta_message_type;
    obj->shield_update_pending = 0;

    if (update_type == 1) {
        if (header.shield_update_pending == 1) {
            baseline.shield_vitality = obj->shield_vitality * 0.33333334f;
        } else {
            baseline.shield_vitality = biped->network_shield_vitality;
        }
        baseline.body_vitality = obj->body_vitality;
        baseline.shield_stunned = (uint8_t)(obj->shield_stun_ticks > 0);
        baseline.grenade_counts = *(int16_t *)&unit->grenade_counts[0]; // object 0x31e as one int16

        slots[0] = &biped->network_grenade_counts; // the delta record starts at 0x52c
        slots[1] = &baseline;
        slots[2] = &header;
        result = message_delta_encode_message(buffer, bit_budget, 1, message_type,
            &slots[2], &slots[1], &slots[0], 1, 0);
    } else {
        slots[1] = &header;
        slots[2] = &biped->network_grenade_counts;
        result = message_delta_encode_message(buffer, bit_budget, 0, message_type,
            &slots[1], &slots[2], 0, 1, 0);
    }

    if (result > 0) {
        biped->network_delta_sequence++;
        if (biped->network_delta_sequence >= 0xff) { // `cmp cl,0xff; jb`: unsigned
            biped->network_delta_sequence = 0;
        }
    }
    unit->network_update_forced = 0;
    return result;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
