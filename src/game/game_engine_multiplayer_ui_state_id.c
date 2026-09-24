// game_engine_multiplayer_ui_state_id  (Ghidra: FUN_004655d0; named speculatively -- see UNSURE)
// address 0x4655d0, size 167 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md gives no summary beyond the raw signature; the shape
//   (pick a base record from network_session or network_client, switch on a "kind" dword at
//   +0x134, and return one of a small set of hardcoded small integers, several of them bumped
//   by +0x1d/+0x1b/+0x20/+0x21 style deltas depending on sub-flags) reads like a UI/menu state
//   selector, not game-simulation logic; renamed accordingly and marked low confidence.
//   network_session (0x0071c2d4) and network_client (0x0071c2d8) are already named externs
//   elsewhere in this module (e.g. src/game/game_engine_apply_variant.c); no header in this
//   repo documents their layout, so every field below is a raw offset.
// register convention: no parameters; pure global-state read.
// UNSURE: this function's name and every field offset into the network session/client record
//   (+0x8, +0xb14, +0x134, +0x180, +0x184, +0x190 = 400) are unresolved; kept as raw offsets.

#include "tags.h"
#include "memory.h"

extern uint8_t *network_session; // 0x0071c2d4
extern uint8_t *network_client;  // 0x0071c2d8

// Selects a small integer id describing the current network session/client "kind" and a few of
// its sub-flags, for UNSURE (likely UI/menu) purposes. UNSURE: see header for every field.
int32_t game_engine_multiplayer_ui_state_id(void)
{
    uint8_t *record;

    if (network_session != (uint8_t *)0) {
        record = network_session + 8;
    } else if (network_client != (uint8_t *)0) {
        record = network_client + 0xb14;
    } else {
        return 8;
    }

    if (record == (uint8_t *)0) {
        return 8;
    }

    switch (*(int32_t *)(record + 0x134)) {
    case 1:
        if (*(uint8_t *)(record + 0x180) == 1) {
            return 0x1d - (*(int32_t *)(record + 0x184) != 0);
        }
        return (-(int32_t)(*(int32_t *)(record + 0x184) != 0) & 0x1b) + 3;
    case 2:
        return 4;
    case 3:
        if (*(int32_t *)(record + 400) == 1) {
            return 0x1f;
        }
        if (*(int32_t *)(record + 400) != 2) {
            return 5;
        }
        return 0x20;
    case 4:
        return 6;
    case 5:
        if (*(int32_t *)(record + 0x180) != 2) {
            return 7;
        }
        return 0x21;
    default:
        return 8;
    }
}

#if 0
Original Ghidra decompilation (0x4655d0), from tools/pack.py 0x4655d0:

int FUN_004655d0(void)

{
  int iVar1;

  if (DAT_0071c2d4 == 0) {
    if (DAT_0071c2d8 == 0) {
      return 8;
    }
    iVar1 = DAT_0071c2d8 + 0xb14;
  }
  else {
    iVar1 = DAT_0071c2d4 + 8;
  }
  if (iVar1 != 0) {
    switch(*(undefined4 *)(iVar1 + 0x134)) {
    case 1:
      if (*(char *)(iVar1 + 0x180) == '\x01') {
        return 0x1d - (uint)(*(int *)(iVar1 + 0x184) != 0);
      }
      return (-(uint)(*(int *)(iVar1 + 0x184) != 0) & 0x1b) + 3;
    case 2:
      return 4;
    case 3:
      if (*(int *)(iVar1 + 400) == 1) {
        return 0x1f;
      }
      if (*(int *)(iVar1 + 400) != 2) {
        return 5;
      }
      return 0x20;
    case 4:
      return 6;
    case 5:
      if (*(int *)(iVar1 + 0x180) != 2) {
        return 7;
      }
      return 0x21;
    }
  }
  return 8;
}
#endif
