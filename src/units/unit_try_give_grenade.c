// unit_try_give_grenade  (Ghidra: FUN_0056d080)
// address 0x56d080, size 214 bytes, name confidence 0.3, rewrite confidence 0.2
// functions.md: "Attempts to give the unit one more grenade of its currently selected type, up
// to the type's maximum, updating related UI/HUD state."
// evidence: types/units.h unit_data.grenade_counts[2] (0x31e); types/objects.h object.flags
//   (0x10, bit 0x4000000 = _object_changed_bit).
// blam-cc: param_1 -> tag_source_index (UNSURE role), unaff_EBX -> unit_index.
// UNSURE: `param_1`'s own tag offset +0x30a exceeds this module's Unit struct (0x2f0 bytes);
// this rewrite keeps it as a raw offset rather than asserting which tag it really indexes
// (possibly a HUD or globals tag reached through a different base than Unit). player_index_from_unit_index and
// equipment_pickup_play_sound's roles (likely "get local player index" and a HUD notification) are not
// recovered either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *globals_tag_data;   // 0x00746fa0
extern data_array *player_data;     // 0x0087a480

extern int32_t player_index_from_unit_index(uint32_t unit_index); // 0x474db0, UNSURE signature
extern void equipment_pickup_play_sound(void);                   // 0x4bbb50, UNSURE signature
extern void object_delete(uint32_t object_index);  // 0x4f5bd0, UNSURE signature

uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index) // blam-cc: param_1, unaff_EBX
{
    uint8_t *tag_data = (uint8_t *)tag_instances[
        ((object_header *)object_data->data)[tag_source_index & 0xffff].data->definition_tag & 0xffff].data;
    int16_t grenade_type = *(int16_t *)(tag_data + 0x30a);
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int16_t *max_count_ptr = (int16_t *)(*(uint8_t **)(globals_tag_data + 300) + grenade_type * 0x44);
    if ((max_count_ptr != (int16_t *)0) && (unit->grenade_counts[grenade_type] < *max_count_ptr)) {
        unit->grenade_counts[grenade_type] += 1;
        unit_obj->flags |= 0x4000000;
        int32_t local_player = player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)player_index_from_unit_index(unit_index);
            if (*(int16_t *)((uint8_t *)player_data->data + (local_player2 & 0xffff) * 0x200 + 2) != -1) {
                equipment_pickup_play_sound();
            }
        }
        object_delete(tag_source_index);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56d080):

undefined4 FUN_0056d080(uint param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  short *psVar5;
  uint unaff_EBX;
  int iVar6;

  iVar3 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar6 = (int)*(short *)(iVar3 + 0x30a);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  uVar2 = 0;
  psVar5 = (short *)(iVar6 * 0x44 + *(int *)(DAT_00746fa0 + 300));
  if ((psVar5 != (short *)0x0) && ((short)*(char *)(iVar6 + 0x31e + iVar1) < *psVar5)) {
    iVar3 = (int)*(short *)(iVar3 + 0x30a);
    *(char *)(iVar3 + 0x31e + iVar1) = *(char *)(iVar3 + 0x31e + iVar1) + '\x01';
    *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x4000000;
    iVar3 = FUN_00474db0();
    if (iVar3 != -1) {
      uVar4 = FUN_00474db0();
      if (*(short *)((uVar4 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1) {
        FUN_004bbb50();
      }
    }
    object_delete();
    uVar2 = 1;
  }
  return uVar2;
}
#endif
