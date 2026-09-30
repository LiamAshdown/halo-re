// item_type_to_message_stage  (Ghidra: FUN_004927c0, unnamed)
// address 0x4927c0, size 109 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Remaps a weapon/equipment type enum to a HUD
// message/animation index used by the pickup-notification logic."; the only caller is
// hud_play_pickup_notification.c, which feeds its input through item_type_to_animation_stage
// next, hence "stage" rather than a final index.
// register convention: item_type_code in AX (in_AX). // blam-cc: item_type_code=AX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// First-stage remap from an item/weapon type code to an intermediate HUD message index; -1 for
// any code with no entry.
int16_t item_type_to_message_stage(int16_t item_type_code)
{
    switch (item_type_code) {
    case 0:  return 6;
    case 1:  return 7;
    case 2:  return 8;
    case 3:  return 9;
    case 4:  return 10;
    case 5:  return 0xb;
    case 6:  return 0xc;
    case 9:  return 0xd;
    case 10: return 0xe;
    case 0xb: return 0x12;
    case 0xc: return 0x13;
    case 0xe: return 4;
    case 0xf: return 1;
    case 0x10: return 0x17;
    case 0x11: return 0x14;
    default: return -1;
    }
}

#if 0
Original Ghidra decompilation (0x4927c0):

undefined4 FUN_004927c0(void)

{
  undefined2 in_AX;

  switch(in_AX) {
  case 0:
    return 6;
  case 1:
    return 7;
  case 2:
    return 8;
  case 3:
    return 9;
  case 4:
    return 10;
  case 5:
    return 0xb;
  case 6:
    return 0xc;
  default:
    return 0xffffffff;
  case 9:
    return 0xd;
  case 10:
    return 0xe;
  case 0xb:
    return 0x12;
  case 0xc:
    return 0x13;
  case 0xe:
    return 4;
  case 0xf:
    return 1;
  case 0x10:
    return 0x17;
  case 0x11:
    return 0x14;
  }
}
#endif
