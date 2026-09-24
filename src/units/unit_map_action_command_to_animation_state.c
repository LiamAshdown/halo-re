// unit_map_action_command_to_animation_state  (Ghidra: FUN_005692b0)
// address 0x5692b0, size 149 bytes, name confidence 0.4, rewrite confidence 0.6
// functions.md: "Maps a scripted/action command enum into the corresponding unit animation-state
// constant and its priority class."
// evidence: types/units.h unit_animation_state (0x1b seat_exit, 0x1c custom_animation-adjacent,
//   0x1d scripted_action, 0x1f/0x21 unknown, 0x29 unknown -- only 0x1b and 0x1d are named there).
// blam-cc: in_CX -> command, in_EDX -> out_priority (may be NULL).
// UNSURE: the two output priority classes (6 and 3) have no established meaning in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority) // blam-cc: in_CX, in_EDX
{
    int32_t state = -1;
    switch (command) {
    case 0: state = 0x1d; break;
    case 1: state = 0x20; break;
    case 2: state = 0x21; break;
    case 3: state = 0x22; break;
    case 4: state = 0x1b; break;
    case 5: state = 0x1c; break;
    case 6: state = 0x1e; break;
    case 7: state = 0x1f; break;
    case 8: state = 4; break;
    case 9: state = 5; break;
    case 10: state = 6; break;
    case 0xb: state = 7; break;
    case 0xc: state = 0x28; break;
    case 0xd: state = 0x29; break;
    }
    if (out_priority != (int16_t *)0) {
        switch (command) {
        case 0: case 1: case 2: case 3: case 6: case 7: case 0xc: case 0xd:
            *out_priority = 6;
            break;
        case 4: case 5: case 8: case 9: case 10: case 0xb:
            *out_priority = 3;
            return state;
        }
    }
    return state;
}

#if 0
Original Ghidra decompilation (0x5692b0):

undefined4 FUN_005692b0(void)

{
  undefined4 uVar1;
  undefined2 in_CX;
  undefined2 *in_EDX;

  uVar1 = 0xffffffff;
  switch(in_CX) {
  case 0:
    uVar1 = 0x1d;
    break;
  case 1:
    uVar1 = 0x20;
    break;
  case 2:
    uVar1 = 0x21;
    break;
  case 3:
    uVar1 = 0x22;
    break;
  case 4:
    uVar1 = 0x1b;
    break;
  case 5:
    uVar1 = 0x1c;
    break;
  case 6:
    uVar1 = 0x1e;
    break;
  case 7:
    uVar1 = 0x1f;
    break;
  case 8:
    uVar1 = 4;
    break;
  case 9:
    uVar1 = 5;
    break;
  case 10:
    uVar1 = 6;
    break;
  case 0xb:
    uVar1 = 7;
    break;
  case 0xc:
    uVar1 = 0x28;
    break;
  case 0xd:
    uVar1 = 0x29;
  }
  if (in_EDX != (undefined2 *)0x0) {
    switch(in_CX) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 6:
    case 7:
    case 0xc:
    case 0xd:
      *in_EDX = 6;
      break;
    case 4:
    case 5:
    case 8:
    case 9:
    case 10:
    case 0xb:
      *in_EDX = 3;
      return uVar1;
    }
  }
  return uVar1;
}
#endif
