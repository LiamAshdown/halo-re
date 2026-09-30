// item_type_to_animation_stage  (Ghidra: FUN_00492880, unnamed)
// address 0x492880, size 164 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Second-stage remap of a weapon/equipment type
// enum into an index used to look up a HUD pickup animation/message entry." Its only caller,
// hud_play_pickup_notification.c, feeds it the return value of item_type_to_message_stage, not
// the original type code (confirmed by disassembly: no register reload between the two calls).
// register convention: message_stage in AX (in_AX). // blam-cc: message_stage=AX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// Second-stage remap from item_type_to_message_stage's result to the final index used to index
// the weapon_hud_interface message table; -1 for any input with no entry.
int16_t item_type_to_animation_stage(int16_t message_stage)
{
    switch (message_stage) {
    case 0:  return 0;
    case 1:  return 0x15;
    case 2:  return 0x16;
    case 3:  return 9;
    case 4:  return 0xc;
    case 5:  return 1;
    case 6:  return 2;
    case 7:  return 0xe;
    case 8:  return 0x12;
    case 9:  return 0x13;
    case 10: return 0xd;
    case 0xb: return 5;
    case 0xc: return 6;
    case 0xd: return 7;
    case 0xe: return 8;
    case 0xf: return 0x17;
    case 0x10: return 0x18;
    case 0x11: return 0x19;
    case 0x12: return 0xb;
    case 0x13: return 10;
    case 0x14: return 0x10;
    case 0x15: return 0x14;
    case 0x16: return 0x1a;
    case 0x17: return 0x1b;
    default: return -1;
    }
}

#if 0
Original Ghidra decompilation (0x492880):

undefined4 FUN_00492880(void)

{
  undefined2 in_AX;

  switch(in_AX) {
  case 0:
    return 0;
  case 1:
    return 0x15;
  case 2:
    return 0x16;
  case 3:
    return 9;
  case 4:
    return 0xc;
  case 5:
    return 1;
  case 6:
    return 2;
  case 7:
    return 0xe;
  case 8:
    return 0x12;
  case 9:
    return 0x13;
  case 10:
    return 0xd;
  case 0xb:
    return 5;
  case 0xc:
    return 6;
  case 0xd:
    return 7;
  case 0xe:
    return 8;
  case 0xf:
    return 0x17;
  case 0x10:
    return 0x18;
  case 0x11:
    return 0x19;
  case 0x12:
    return 0xb;
  case 0x13:
    return 10;
  case 0x14:
    return 0x10;
  case 0x15:
    return 0x14;
  case 0x16:
    return 0x1a;
  case 0x17:
    return 0x1b;
  default:
    return 0xffffffff;
  }
}
#endif
