// unit_any_dying_or_seat_transition  (Ghidra: FUN_0056c070)
// address 0x56c070, size 124 bytes, name confidence 0.3, rewrite confidence 0.5
// functions.md: "Scans all units for one that is dead (with a specific sub-state) or currently
// entering/exiting a seat, returning true if any match."
// evidence: types/units.h unit_data.animation_state (0x2a3, 0x21 = throwing_grenade, 0x18/0x19 =
//   unknown/ready_weapon), .throwing_grenade_state (0x28d), .animation_state_flags (0x298, bit
//   0x4 = unknown_4).
// blam-cc: (no visible parameters; scans all objects).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object * object_iterator_next(object_iterator *iterator); // 0x4f6f20

uint8_t unit_any_dying_or_seat_transition(void)
{
    object_iterator iter = { _object_mask_unit, 1, 0, 0, 0xffffffff };
    object *obj = object_iterator_next(&iter);
    while (obj != (object *)0) {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        if (((unit->animation_state == 0x21) && (unit->throwing_grenade_state != 3)) ||
            (((unit->animation_state == 0x19) || (unit->animation_state == 0x18)) &&
             ((unit->animation_state_flags & 4) == 0))) {
            return 1;
        }
        obj = object_iterator_next(&iter);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56c070):

undefined4 FUN_0056c070(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 3;
  local_c = 1;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar2 = object_iterator_next(&local_10);
  while( true ) {
    if (iVar2 == 0) {
      return 0;
    }
    cVar1 = *(char *)(iVar2 + 0x2a3);
    if (((cVar1 == '!') && (*(char *)(iVar2 + 0x28d) != '\x03')) ||
       (((cVar1 == '\x19' || (cVar1 == '\x18')) && ((*(byte *)(iVar2 + 0x298) & 4) == 0)))) break;
    iVar2 = object_iterator_next(&local_10);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
