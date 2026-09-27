// unit_drop_grenades  (Ghidra: already named unit_drop_grenades)
// address 0x56ef60, size 246 bytes
// name confidence: 0.75 (cea-pdb hint 'unit_drop_grenades'; functions.md summary matches)
// rewrite confidence: 0.35
// evidence: types/units.h unit_data.grenade_counts (0x31e); types/objects.h
//   object_placement_data (size 0x88, matches the Ghidra local buffer exactly); callees
//   object_placement_data_initialize (0x4f53a0), object_new_with_datum_role_control (0x4f54b0),
//   unit_drop_object_from_hand (0x56ed00, this batch).
// UNSURE: the grenade-type table at globals_tag_data+0x12c (same table read by
//   unit_throw_grenade_move_to_hand, there at field +0x40; here at field +0x30 -- a different
//   field of the same 0x44-byte record) is not named in types/units.h.
// UNSURE: the role-override check (indexing object_type_definitions by the spawned tag's own
//   object_type and testing its +0x10 field) is reproduced by raw offset; its exact meaning
//   (likely "does this object type replicate itself over the network") is not resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *globals_tag_data;    // 0x00746fa0
extern int16_t game_connection_role; // 0x00719720 (a WORD; 0x719722 is the screenshot counter)
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc, an ARRAY (was a pointer variable)

extern void object_placement_data_initialize(object_placement_data *placement,
                                              datum_index definition_tag, datum_index role); // 0x4f53a0
extern uint32_t object_new_with_datum_role_control(object_placement_data *placement,
                                                    uint32_t role); // 0x4f54b0
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00

// Spawns and drops all of the unit's carried grenades of both types into the world (e.g. on
// death), decrementing each grenade_counts slot to zero.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> unit_index
void unit_drop_grenades(uint32_t unit_index)
{
    uint8_t *grenade_type_table = *(uint8_t **)(globals_tag_data + 0x12c); // UNSURE table identity
    int32_t table_offset = 0;
    int grenade_type;

    for (grenade_type = 0; grenade_type < 2; grenade_type++) {
        object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        int8_t *count = &unit->grenade_counts[grenade_type];

        while (*count > 0) {
            object_placement_data placement;
            datum_index projectile_tag = *(datum_index *)(grenade_type_table + table_offset + 0x30);
            uint32_t role = 3;

            object_placement_data_initialize(&placement, projectile_tag, unit_index);

            if (game_connection_role == 2) {
                Object *proj_tag = (Object *)tag_instances[placement.definition_tag & 0xffff].data;
                object_type_definition *type_def = object_type_definitions[proj_tag->object_type];
                if (*(int32_t *)((uint8_t *)type_def + 0x10) != -1) {
                    role = 0;
                }
            }

            {
                uint32_t projectile_index = object_new_with_datum_role_control(&placement, role);
                if (projectile_index != 0xffffffff) {
                    object_unlink_cluster_or_notify_parent(projectile_index);
                    unit_drop_object_from_hand(unit_index, projectile_index);
                }
            }

            *count -= 1;
        }
        table_offset += 0x44;
    }
}

#if 0
Original Ghidra decompilation (0x56ef60):

void unit_drop_grenades(uint param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  char cVar4;
  char *pcVar5;
  int local_90;
  int local_8c;
  uint local_88 [34];

  local_90 = 0;
  pcVar5 = (char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x31e);
  local_8c = 2;
  do {
    iVar1 = *(int *)(DAT_00746fa0 + 300);
    cVar4 = *pcVar5;
    while ('\0' < cVar4) {
      object_placement_data_initialize(*(undefined4 *)(iVar1 + local_90 + 0x30),param_1);
      uVar2 = 3;
      if ((DAT_00719720 == 2) &&
         (*(int *)((&PTR_PTR_0069bfdc)
                   [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10) != -1
         )) {
        uVar2 = 0;
      }
      iVar3 = object_new_with_datum_role_control(local_88,uVar2);
      if (iVar3 != -1) {
        object_unlink_cluster_or_notify_parent();
        unit_drop_object_from_hand(param_1,iVar3);
      }
      cVar4 = *pcVar5 + -1;
      *pcVar5 = cVar4;
    }
    local_90 = local_90 + 0x44;
    pcVar5 = pcVar5 + 1;
    local_8c = local_8c + -1;
  } while (local_8c != 0);
  return;
}
#endif
