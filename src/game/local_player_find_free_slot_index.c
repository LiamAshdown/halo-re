// local_player_find_free_slot_index  (Ghidra: FUN_00473730; named per this rewrite)
// address 0x473730, size 73 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: the only caller in the image, 0x4c8800 (main module), passes this function's return
//   value straight to game_engine_create_player as a local_player_index and then bounds-checks
//   it with "-1 < sVar7 && sVar7 < 1" before indexing player_globals::local_players -- i.e. the
//   caller treats the result as a local-player slot, not a team. out/phase4/game_functions.md
//   guessed "Finds an available (empty or unassigned) team slot for assigning a new player"
//   (conf 0.45); CORRECTED here to local-player slot, matching players_any_with_local_player_index
//   (0x4736d0, this batch), which this function's every call site actually tests.
// register convention: no arguments; return value in EAX.
//
// UNSURE (superseded by R02, see below): types/game.h used to call the global at 0x006b2ce8
// "team_slot_table" and describe this function as scanning it "for a free team". The disassembly shows it is compared against
// players' local_player_index (offset 0x02), never against player::team (offset 0x20), and the
// scan only ever covers 4 slots while k_maximum_local_players is 1 in this build -- this reads
// like Xbox-era up-to-4-controller local-player bookkeeping that survived into the PC port
// unused. R02: the global is input.h's joystick_slot_devices[4] (slot -> input device, -1 none);
// a slot is "hinted" when a joystick is mapped to it.
// The dead tail the original compiles to (the fast-scan result is always in 0..3, so the
// "if (result != -1)" it re-tests can never take the slow path it guards) is folded away below;
// see the #if 0 block for exactly how Ghidra expressed it.
// reconciled: R02 0x006b2ce8 local_player_slot_hint_table -> input.h joystick_slot_devices[4]

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t joystick_slot_devices[4]; // 0x006b2ce8, input.h (slot -> device index, -1 none)

extern uint8_t players_any_with_local_player_index(int16_t local_player_index); // this batch,
    // 0x4736d0, blam-cc: ESI -> local_player_index

// Picks a free local-player slot index (0..3). Prefers a slot that has a joystick mapped
// and that no live player currently occupies; if none of the four hinted slots is free, falls
// back to the first slot with no live player at all; returns -1 if every slot is taken.
int32_t local_player_find_free_slot_index(void)
{
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        if (joystick_slot_devices[i] != -1 &&
            players_any_with_local_player_index((int16_t)i) == 0) {
            return i;
        }
    }
    for (i = 0; i < 4; i = i + 1) {
        if (players_any_with_local_player_index((int16_t)i) == 0) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x473730), from tools/pack.py 0x473730:

int FUN_00473730(void)

{
  char cVar1;
  int iVar2;
  int iVar3;

  iVar3 = -1;
  iVar2 = 0;
  while (((&DAT_006b2ce8)[(short)iVar2] == -1 || (cVar1 = FUN_004736d0(), cVar1 != '\0'))) {
    iVar2 = iVar2 + 1;
    if (3 < iVar2) {
LAB_00473765:
      iVar2 = 0;
      do {
        cVar1 = FUN_004736d0();
        if (cVar1 == '\0') {
          return iVar2;
        }
        iVar2 = iVar2 + 1;
      } while (iVar2 < 4);
      return iVar3;
    }
  }
  iVar3 = iVar2;
  if (iVar2 != -1) {
    return iVar2;
  }
  goto LAB_00473765;
}
#endif
