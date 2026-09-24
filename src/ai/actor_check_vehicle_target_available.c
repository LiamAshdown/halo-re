// actor_check_vehicle_target_available  (Ghidra: actor_check_vehicle_target_available; named for this rewrite)
// address 0x42b810, size 111 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: phase-4 summary ("checks whether an actor's vehicle target is currently
// available and, if not, optionally flags the actor to pursue boarding it"); teams_are_enemies
// (0x45bd50) is already established elsewhere in this module as taking untraced CX/DX team
// registers, matching its use here.
// register convention: EAX -> vehicle_object_index, ECX -> actor_index; stack ->
// flag_pursue.
// blam-cc: EAX -> vehicle_object_index, ECX -> actor_index, stack -> flag_pursue
//
// UNSURE: teams_are_enemies's two team-index inputs are not traced by this function or by
// the other two call sites already in this module; presumed to be a caller-to-callee
// register passthrough this function's own decompile never surfaces.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern int8_t teams_are_enemies(void); // 0x45bd50, teams_are_enemies; UNSURE: no traced args here either

// blam-cc: EAX -> vehicle_object_index, ECX -> actor_index, stack -> flag_pursue
// Returns 1 only when vehicle_object_index is valid, its unit extension's
// controlling_player is set, and teams_are_enemies says that controller's team is not
// hostile; returns 0 otherwise (including when nothing currently controls the vehicle).
// UNSURE: this reads backward from "is the vehicle free for the actor to board" -- the
// gate is on controlling_player being SET, not clear -- so controlling_player at unit+0x218
// may not be the right field name for what this function actually tests; kept for
// consistency with types/units.h until cross-checked. When flag_pursue is set and the check
// passes, marks the actor as pursuing it.
uint8_t actor_check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index,
                                              uint8_t flag_pursue)
{
    object *vehicle_object;
    unit_data *vehicle_unit;
    actor *self;

    if (vehicle_object_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    vehicle_object = ((object_header *)object_data->data)[vehicle_object_index & 0xffff].data;
    vehicle_unit = (unit_data *)((uint8_t *)vehicle_object + k_unit_data_offset);
    if (vehicle_unit->controlling_player == (datum_index)k_datum_index_none) {
        return 0;
    }
    if (teams_are_enemies() != 0) {
        return 0;
    }
    if (flag_pursue) {
        self = &((actor *)actor_data->data)[actor_index & 0xffff];
        self->unknown_2ed = 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x42b810):

undefined4 FUN_0042b810(char param_1)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  uint in_ECX;

  if ((in_EAX != 0xffffffff) &&
     (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x218) != -1)
     ) {
    iVar1 = *(int *)(DAT_00880360 + 0x34);
    cVar2 = FUN_0045bd50();
    if (cVar2 == '\0') {
      if (param_1 != '\0') {
        *(undefined1 *)((in_ECX & 0xffff) * 0x724 + iVar1 + 0x2ed) = 1;
      }
      return 1;
    }
    return 0;
  }
  return 0;
}
#endif
