// unit_refresh_targeting_flag_and_weapons  (Ghidra: FUN_00569bf0)
// address 0x569bf0, size 158 bytes, name confidence 0.3, rewrite confidence 0.9 (verified against objdump 0x569bf0..0x569c8d)
// functions.md: "Recomputes a control/targeting flag on the unit based on whether it has any
// active target or seat references, then refreshes each carried weapon and the unit's occupant
// tracking."
// evidence: types/units.h unit_data.actor_index (0x1f4), .swarm_actor_index (0x1f8),
//   .controlling_player (0x218), .weapons[4] (0x2f8), .flags (0x204, bit 0 = _unit_flag_unattended
//   per the enum comment -- see UNSURE below).
// blam-cc: param_1 -> unit_index, unaff_CL -> initial_targeting_flag.
// UNSURE: `in_CL` is read in the original before ever being written on the "none of
// actor/swarm/controlling_player are set" path -- a genuine uninitialized-register read in
// Ghidra's model, reproduced here as an explicit incoming parameter. types/units.h's comment on
// _unit_flag_unattended (bit 0x1, "sets it when the unit has neither an actor nor a swarm
// reference") describes the opposite polarity of what this function's visible arithmetic shows
// (bit 0x1 is set when the OR of the three conditions is true, assuming the ambient CL value is
// 0 on entry); this rewrite preserves the literal arithmetic without asserting which reading is
// correct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void item_set_holder(uint32_t item_index, datum_index holder_index); // 0x4bcfc0, ECX item, EDX holder
extern void unit_recompute_seat_occupants(uint32_t unit_index);              // 0x56ce30

void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag) // blam-cc: param_1, unaff_CL
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    uint8_t has_reference = initial_targeting_flag;

    if ((unit->actor_index != k_datum_index_none) || (unit->swarm_actor_index != k_datum_index_none) ||
        (unit->controlling_player != k_datum_index_none)) {
        has_reference = 1;
    }

    uint32_t flags = unit->flags;
    if (((unit_obj->vitality_flags & _object_health_frozen_bit) == 0) && has_reference) {
        unit->flags = flags | 1;
        flags |= 0x41;
    } else {
        unit->flags = flags & 0xfffffffe;
        flags &= 0xffffffbe;
    }
    unit->flags = flags;

    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        if (unit->weapons[i] != k_datum_index_none) {
            item_set_holder(unit->weapons[i], unit_index); // 0x569c77: ECX = weapon, EDX = ebx = unit_index
        }
    }
    unit_recompute_seat_occupants(unit_index);
    return;
}

#if 0
Original Ghidra decompilation (0x569bf0):

void FUN_00569bf0(uint param_1)

{
  char in_CL;
  uint uVar1;
  int *piVar2;
  int iVar3;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((*(int *)(iVar3 + 500) != -1) || (*(int *)(iVar3 + 0x1f8) != -1)) ||
     (*(int *)(iVar3 + 0x218) != -1)) {
    in_CL = '\x01';
  }
  if (((*(byte *)(iVar3 + 0x106) & 4) == 0) && (in_CL != '\0')) {
    uVar1 = *(uint *)(iVar3 + 0x204);
    *(uint *)(iVar3 + 0x204) = uVar1 | 1;
    uVar1 = uVar1 | 0x41;
  }
  else {
    uVar1 = *(uint *)(iVar3 + 0x204);
    *(uint *)(iVar3 + 0x204) = uVar1 & 0xfffffffe;
    uVar1 = uVar1 & 0xffffffbe;
  }
  *(uint *)(iVar3 + 0x204) = uVar1;
  piVar2 = (int *)(iVar3 + 0x2f8);
  iVar3 = 4;
  do {
    if (*piVar2 != -1) {
      FUN_004bcfc0();
    }
    piVar2 = piVar2 + 1;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  FUN_0056ce30();
  return;
}
#endif
