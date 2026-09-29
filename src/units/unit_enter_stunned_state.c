// unit_enter_stunned_state  (Ghidra: FUN_005705a0; renamed from the phase2 proposal)
// address 0x5705a0, size 175 bytes
// name confidence: 0.45 (phase2 proposal at 0.45, matches functions.md summary)
// rewrite confidence: 0.4
// evidence: types/units.h unit_data.flags (0x204, bit 0x80 = _unit_flag_disoriented),
//   .unknown_28b (0x28b, "countdown; 0x5705a0 seeds it with a random stun duration"),
//   .unknown_410 (0x410, "stored by 0x5705a0, read back by 0x570720"); types/objects.h
//   object.vitality_flags (0x106); callees unit_drop_current_weapon (0x56dec0), 0x570650
//   (this batch).
// register convention: unit object index in EDI (unaff_EDI); the responsible object index in a
//   stack parameter (param_1).
//   // blam-cc: EDI -> unit_index, stack -> responsible_object
// UNSURE: unit_drop_current_weapon's second argument is not visible at this call site; passed
//   as 1, matching every other call to it in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern random_seed random_seed_global;   // 0x00719cd0

extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern void unit_initialize_random_turn_angle(uint32_t object_index); // 0x570650, this batch

// Puts the unit into a disoriented/stunned state: drops its current weapon, sets the
// disoriented flag and adjusts vitality flags, and -- if not already stunned -- picks a random
// stun duration, records the responsible object, and kicks off the random idle-turn wander.
void unit_enter_stunned_state(uint32_t unit_index, uint32_t responsible_object)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    unit_drop_current_weapon(unit_index, 1);
    unit->flags |= _unit_flag_disoriented;
    obj->vitality_flags = (obj->vitality_flags & 0xfffb) | 0x800;

    if (unit->flaming_ticks == 0) {
        int16_t duration;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        duration = (int16_t)(((int32_t)(random_seed_global >> 0x10) * 0x5a) >> 0x10) + 0x3c;

        if (duration == 0) {
            duration = 1;
        } else if (duration > 0xff) {
            duration = 0xff;
        }

        unit->flaming_ticks = (int8_t)duration;
        unit->flaming_responsible_object = responsible_object;
        unit_initialize_random_turn_angle(unit_index);
    }
}

#if 0
Original Ghidra decompilation (0x5705a0):

void FUN_005705a0(undefined4 param_1)

{
  int iVar1;
  undefined1 uVar2;
  ushort uVar3;
  uint unaff_EDI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  unit_drop_current_weapon();
  *(uint *)(iVar1 + 0x204) = *(uint *)(iVar1 + 0x204) | 0x80;
  *(ushort *)(iVar1 + 0x106) = *(ushort *)(iVar1 + 0x106) & 0xfffb | 0x800;
  if (*(char *)(iVar1 + 0x28b) == '\0') {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar3 = (short)((random_seed_global >> 0x10) * 0x5a >> 0x10) + 0x3c;
    uVar2 = (undefined1)uVar3;
    if (uVar3 == 0) {
      uVar2 = 1;
    }
    else if (0xff < uVar3) {
      uVar2 = 0xff;
    }
    *(undefined1 *)(iVar1 + 0x28b) = uVar2;
    *(undefined4 *)(iVar1 + 0x410) = param_1;
    FUN_00570650();
  }
  return;
}
#endif
