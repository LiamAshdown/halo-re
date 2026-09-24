// unit_exit_vehicle_seat  (Ghidra: unit_exit_vehicle_seat, already named)
// address 0x568120, size 263 bytes
// name confidence: 0.5 (already carries this name; matches functions.md's summary: "Removes a
//   unit from its current vehicle seat, resetting its pose and transient state and broadcasting
//   the change")   rewrite confidence: 0.3
// evidence: types/memory.h data_array (maximum_count 0x20, size 0x22, data 0x34); the player
//   data_array at 0x0087a480 with the controlled-unit handle at record+0x34 (see
//   src/units/unit_all_seats_unoccupied.c and out/phase4/units_types_notes.md).
// register convention: player index (with salt in the high 16 bits) in EAX.
//   // blam-cc: in_EAX -> player_index
// UNSURE: the 8-dword record copied into `auStack_54` and handed to unit_broadcast_state_change_event could not be
//   given real field names; it is reproduced byte-for-byte as the original assigns it (a unit
//   handle, seven flag/sentinel bytes, a 0xffff sentinel short, a zero dword, and the player
//   record's own +0x2c field), matching the same unresolved record this module's other
//   unit_broadcast_state_change_event caller (unit_apply_damage_effects) builds.
// UNSURE: the first, all-zero-except-slot-5 build of `auStack_54` (before the second build
//   overwrites it) is dead in the original -- every one of its 8 words is unconditionally
//   overwritten before the first read -- and is reproduced only for a literal diff against the
//   Ghidra output, not because it has any effect.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
                                        uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction,
                                        float turn_angle, int16_t weapon_class_index, int32_t fire_trigger_event,
                                        uint8_t require_still); // 0x566de0
extern void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset); // 0x568610, UNSURE signature
extern void unit_broadcast_state_change_event(int32_t index);                     // 0x566c00, UNSURE signature
extern void network_index_cache_remove(uint32_t object_index);                 // 0x4e9d40, UNSURE signature

void unit_exit_vehicle_seat(uint32_t player_index) // blam-cc: in_EAX -> player_index
{
    if ((player_index != 0xffffffff) && (0 <= (int16_t)player_index) &&
        ((int16_t)player_index < player_data->maximum_count)) {
        int32_t record_offset = (int32_t)player_data->size * (int16_t)player_index;
        int16_t salt = *(int16_t *)((uint8_t *)player_data->data + record_offset);
        int16_t requested_salt = (int16_t)(player_index >> 16);
        if ((salt != 0) && ((requested_salt == 0) || (salt == requested_salt))) {
            uint32_t unit_index = *(uint32_t *)((uint8_t *)player_data->data + record_offset + 0x34);

            uint32_t record[8] = { 0, 0, 0xffffffff, 0, 0, 0, 0, 0 }; // dead first build, see header

            unit_update_stance_and_jump(unit_index, 1, 0, 0, 0, 0, 0.0f, -1, 0, 0);
            unit_release_transient_state(unit_index, 0);

            uint32_t player_field_2c = *(uint32_t *)((uint8_t *)player_data->data + record_offset + 0x2c);

            uint8_t *rec_bytes = (uint8_t *)record;
            record[0] = unit_index;
            rec_bytes[4] = 1;
            rec_bytes[5] = 1;
            rec_bytes[6] = 0;
            rec_bytes[7] = 0;
            rec_bytes[8] = 0;
            rec_bytes[9] = 0;
            rec_bytes[10] = 1;
            *(int16_t *)(rec_bytes + 12) = -1; // 0xffff
            *(int32_t *)(rec_bytes + 16) = 0;
            record[7] = player_field_2c;

            unit_broadcast_state_change_event((int32_t)record);

            object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            obj->network_role = 3;
            if ((((object_header *)object_data->data)[unit_index & 0xffff].flags & 8) == 0) {
                network_index_cache_remove(unit_index);
            }
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x568120):

void unit_exit_vehicle_seat(void)

{
  uint uVar1;
  short sVar2;
  int in_EAX;
  int iVar3;
  short sVar4;
  int iVar5;
  uint *puVar6;
  uint *puVar7;
  uint auStack_54 [8];
  uint local_24;
  undefined1 local_20;
  undefined1 local_1f;
  undefined1 local_1e;
  undefined1 local_1d;
  undefined1 local_1c;
  undefined1 local_1b;
  undefined1 local_1a;
  undefined2 local_18;
  undefined4 local_14;
  undefined4 local_8;

  if (((in_EAX != -1) && (sVar2 = (short)in_EAX, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2;
    sVar2 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar2 != 0) && ((sVar4 = (short)((uint)in_EAX >> 0x10), sVar4 == 0 || (sVar2 == sVar4)))) {
      uVar1 = *(uint *)(iVar5 + 0x34);
      auStack_54[7] = 0;
      auStack_54[6] = 0;
      auStack_54[5] = 0xffffffff;
      auStack_54[4] = 0;
      auStack_54[3] = 0;
      auStack_54[2] = 0;
      auStack_54[1] = 0;
      auStack_54[0] = 0;
      FUN_00566de0(uVar1,1);
      FUN_00568610(uVar1,0);
      local_8 = *(undefined4 *)(iVar5 + 0x2c);
      local_20 = 1;
      local_1f = 1;
      local_1e = 0;
      local_1d = 0;
      local_1c = 0;
      local_1b = 0;
      local_18 = 0xffff;
      local_14 = 0;
      local_1a = 1;
      local_24 = uVar1;
      puVar6 = &local_24;
      puVar7 = auStack_54;
      for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar7 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
      }
      FUN_00566c00();
      iVar5 = DAT_008603b0;
      iVar3 = (uVar1 & 0xffff) * 0xc;
      *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar3) + 4) = 3;
      if ((*(byte *)(*(int *)(iVar5 + 0x34) + 2 + iVar3) & 8) == 0) {
        auStack_54[7] = 0x56821f;
        FUN_004e9d40();
      }
    }
  }
  return;
}
#endif
