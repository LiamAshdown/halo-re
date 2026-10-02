// unit_dispatch_seat_overlay_command  (Ghidra: FUN_00567400)
// address 0x567400, size 123 bytes
// name confidence: 0.35 (matches functions.md's summary: "Dispatches a seat-control command to
//   the appropriate overlay-animation starter based on its enumerated value")
// rewrite confidence: 0.6
// evidence: the two callees, unit_start_seat_overlay_animation_a (0x565e00, unit+0x2aa) and
//   unit_start_seat_overlay_animation_b (0x566410, unit+0x2ae), already established as taking
//   (unit_index in EAX, command in CX/param_2).
// register convention: unit index in EAX (passed through untouched), command in CX.
//   // blam-cc: in_EAX -> unit_index, in_CX -> command
// UNSURE: command == 3 falls through to a plain `return` instead of an early one in the
//   original; reproduced identically even though it has no observable difference here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command); // 0x565e00
extern void unit_start_seat_overlay_animation_b(uint32_t unit_index, int16_t command); // 0x566410

void unit_dispatch_seat_overlay_command(uint32_t unit_index, int16_t command) // blam-cc: in_EAX -> unit_index, in_CX -> command
{
    switch (command) {
    case 1:
        unit_start_seat_overlay_animation_b(unit_index, command);
        return;
    case 2:
        unit_start_seat_overlay_animation_b(unit_index, command);
        return;
    case 3:
        unit_start_seat_overlay_animation_b(unit_index, command);
        break;
    case 4:
        unit_start_seat_overlay_animation_b(unit_index, command);
        return;
    case 5:
        unit_start_seat_overlay_animation_a(unit_index, command);
        return;
    case 6:
        unit_start_seat_overlay_animation_a(unit_index, command);
        return;
    case 7:
        unit_start_seat_overlay_animation_b(unit_index, command);
        return;
    case 8:
        unit_start_seat_overlay_animation_b(unit_index, command);
        return;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x567400):

void FUN_00567400(void)

{
  undefined2 in_CX;

  switch(in_CX) {
  case 1:
    FUN_00566410();
    return;
  case 2:
    FUN_00566410();
    return;
  case 3:
    FUN_00566410();
    break;
  case 4:
    FUN_00566410();
    return;
  case 5:
    FUN_00565e00();
    return;
  case 6:
    FUN_00565e00();
    return;
  case 7:
    FUN_00566410();
    return;
  case 8:
    FUN_00566410();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
