// player_profile_auto_select  (Ghidra: FUN_004952c0, unnamed)
// address 0x4952c0, size 174 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: out/phase4/interface_functions.md "Attempts to automatically select and load an
// existing player profile at startup, falling back to a default (-1) profile if none is found
// valid."; src/game/game_engine_get_variant_by_name.c's saved_game_enumerate_by_type
// convention (EBX -> capacity-then-count int32).
// register convention: none (void).
// Review pass (phase 4): rebuilt from the disassembly. Ghidra's `int local_2000[2047]` is really
// the 0x2004 byte frame {int32 count; int32 slot; uint8 profile[0x1ffc]}: EBX points at the count
// (1 in), the slot array is one entry long, and the profile record at +8 is what
// player_profile_get fills (ECX) and player_profile_load copies (EDX, player index 0 in EAX,
// slot on the stack). Each enumeration is only trusted when its returned count is positive.
// UNSURE: player_profile_get / saved_game_enumerate_by_type are owned by the profile module
// (0x0053a000 region) and not rewritten in this tree.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern void saved_game_enumerate_by_type(int32_t type, int32_t *out_slots, int32_t unknown);
    // 0x53c4e0; blam-cc: EBX -> capacity-then-count int32
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970

// Looks for an existing player profile at startup: tries a type-0 enumeration with flag 0 first
// and loads that slot if it validates; otherwise (nothing found) tries flag 1 and, if that slot
// validates, loads its record as the default (-1) profile. Does nothing if neither validates.
void player_profile_auto_select(void)
{
    int32_t count;                 // esp+0x08 in the body, frame +0
    int32_t slot;                  // frame +4
    uint8_t profile_data[0x1ffc];  // frame +8

    count = 1;
    slot = -1;
    saved_game_enumerate_by_type(0, &slot, 0); // EBX = &count
    if ((int16_t)count > 0 && slot != -1) {
        if (player_profile_get(slot, profile_data) == 0) {
            return;
        }
        player_profile_load(0, profile_data, slot);
        return;
    }

    count = 1;
    saved_game_enumerate_by_type(0, &slot, 1); // EBX = &count
    if ((int16_t)count <= 0 || slot == -1) {
        return;
    }
    if (player_profile_get(slot, profile_data) == 0) {
        return;
    }
    player_profile_load(0, profile_data, -1);
}

#if 0
Original Ghidra decompilation (0x4952c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004952c0(void)

{
  char cVar1;
  int iVar2;
  int local_2000 [2047];
  undefined4 uStack_4;

  uStack_4 = 0x4952ca;
  local_2000[0] = -1;
  saved_game_enumerate_by_type(0,local_2000,0);
  iVar2 = local_2000[0];
  if (local_2000[0] == -1) {
    saved_game_enumerate_by_type(0,local_2000,1);
    if (local_2000[0] == -1) {
      return;
    }
    cVar1 = player_profile_get(local_2000[0]);
    if (cVar1 == '\0') {
      return;
    }
    iVar2 = -1;
  }
  else {
    cVar1 = player_profile_get(local_2000[0]);
    if (cVar1 == '\0') {
      return;
    }
  }
  player_profile_load(iVar2);
  return;
}
#endif
