// unit_update_autoaim_interaction  (Ghidra: FUN_00570720; renamed from the phase2 proposal)
// address 0x570720, size 277 bytes
// name confidence: 0.25 (phase2 proposal at 0.25, matches functions.md summary)
// rewrite confidence: 0.3
// evidence: types/units.h unit_data.flags (0x204, bits 0x2000000 and 0x80 cleared),
//   .unknown_410 (0x410, "stored by 0x5705a0, read back by 0x570720"); types/objects.h
//   object.vitality_flags (0x106), object.owner_linkage (0x0c0), .creator_object (0x0c4),
//   .name_index (0x0b8); the damage_data field mapping follows unit_cause_melee_damage.c.
// register convention: unit object index in EAX (param_1).
//   // blam-cc: EAX -> unit_index
// UNSURE: globals_tag_data+0x18c is treated here as a pointer to a small record whose +0x78 is
//   a globally tracked target object index and whose base address doubles as the damage effect
//   tag -- this is a different reading of the same offset from unit_apply_fall_damage.c's "fall
//   damage table", and the two could not be reconciled in the time available.
// UNSURE: object_try_and_get's register object-index argument is not visible; guessed as the
//   same tracked target index read from the globals record.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;   // 0x008603b0
extern uint8_t *globals_tag_data; // 0x00746fa0

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t param_3,
                                 int16_t param_4, int16_t param_5, uint32_t param_6); // 0x4ee5e0

// Clears the disoriented and idle-turn-seeded flags for the unit each tick and, if a global
// interaction target device is set, applies a damage/interaction tick to it.
void unit_update_autoaim_interaction(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *tracked = *(uint8_t **)(globals_tag_data + 0x18c);

    unit->flags &= ~0x02000000u;
    obj->vitality_flags &= 0xfff7; // UNSURE: clears object+0x107 bit 3
    unit->flags &= ~0x00000080u;

    if (tracked != 0) {
        datum_index target_object = *(datum_index *)(tracked + 0x78);
        if (target_object != k_datum_index_none) {
            object *target = object_try_and_get(target_object, 0xffffffff);
            damage_data dd = {0};

            dd.team_index = -1;
            dd.responsible_object = k_datum_index_none;
            dd.responsible_player = -1;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;

            if (target != 0) {
                dd.responsible_player = target->owner_linkage;
                dd.responsible_object = target->creator_object;
                if (dd.responsible_object == k_datum_index_none) {
                    dd.responsible_object = unit->unknown_410;
                }
                dd.team_index = target->owner_team;
            }
            dd.damage_effect_tag = target_object; // literal reuse of the same value, see header
            object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
        }
    }

    if ((obj->vitality_flags & 4) == 0) {
        obj->vitality_flags |= 0x20;
    }
}

#if 0
Original Ghidra decompilation (0x570720):

void FUN_00570720(uint param_1)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int local_58 [4];
  undefined2 local_48;
  undefined2 local_40;
  undefined4 local_18;
  undefined4 local_14;
  undefined2 local_c;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar2 = *(uint *)(iVar1 + 0x204);
  iVar3 = *(int *)(DAT_00746fa0 + 0x18c);
  *(uint *)(iVar1 + 0x204) = uVar2 & 0xfdffffff;
  *(byte *)(iVar1 + 0x107) = *(byte *)(iVar1 + 0x107) & 0xf7;
  *(uint *)(iVar1 + 0x204) = uVar2 & 0xfdffff7f;
  if ((iVar3 != 0) && (iVar3 = *(int *)(iVar3 + 0x78), iVar3 != -1)) {
    iVar4 = object_try_and_get(0xffffffff);
    piVar6 = local_58;
    for (iVar5 = 0x15; iVar5 != 0; iVar5 = iVar5 + -1) {
      *piVar6 = 0;
      piVar6 = piVar6 + 1;
    }
    local_c = 0xffff;
    local_58[2] = 0xffffffff;
    local_58[3] = -1;
    local_48 = 0xffff;
    local_40 = 0xffff;
    local_18 = 0x3f800000;
    local_14 = 0x3f800000;
    if (iVar4 != 0) {
      local_58[2] = *(undefined4 *)(iVar4 + 0xc0);
      local_58[3] = *(int *)(iVar4 + 0xc4);
      if (local_58[3] == -1) {
        local_58[3] = *(int *)(iVar1 + 0x410);
      }
      local_48 = *(undefined2 *)(iVar4 + 0xb8);
    }
    local_58[0] = iVar3;
    object_apply_damage(local_58,param_1,0xffffffff,0xffffffff,0xffffffff,0);
  }
  if ((*(ushort *)(iVar1 + 0x106) & 4) == 0) {
    *(ushort *)(iVar1 + 0x106) = *(ushort *)(iVar1 + 0x106) | 0x20;
  }
  return;
}
#endif
