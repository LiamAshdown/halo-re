// network_machine_clear_flag_by_id  (Ghidra: FUN_004e0b90, unnamed)
// address 0x4e0b90, size 63 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Finds the machine-table slot matching a given
// id and clears a flag/name byte at offset 0x50 within that slot." Matches
// network_machine::unknown_50 exactly.
// register convention: EDX = server (network_server_globals *), EDI = machine_id (int32_t).
// blam-cc: EDX -> server, EDI -> machine_id
// UNSURE: the low byte of the returned pointer is masked off (`& 0xffffff00`), matching the
// same odd idiom seen in network_game_session_finalize_and_add_player.c; preserved literally.
// UNSURE: when no match is found, the original writes a zero byte to the fixed absolute
// address 0x50 (`DAT_00000050 = 0`) rather than through any parameter -- almost certainly a
// decompilation artifact (a register Ghidra treated as a bare small-integer global), but
// transcribed exactly as shown per the no-invented-behaviour rule.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t DAT_00000050; // UNSURE: see header note; likely a decompilation artifact

// Finds the machines[] slot whose machine_id equals machine_id and clears its unknown_50
// byte, returning that slot's address (with its low byte masked off). If no slot matches,
// writes a zero byte to absolute address 0x50 instead (see UNSURE note).
uint32_t network_machine_clear_flag_by_id(network_server_globals *server, int32_t machine_id)
{
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            server->machines[i].player_joined = 0;
            return ((uint32_t)&server->machines[i]) & 0xffffff00;
        }
    }
    DAT_00000050 = 0;
    return ((uint32_t)16) & 0xffffff00; // UNSURE: original's uVar1 (loop counter, 0x10) masked
}

#if 0
Original Ghidra decompilation (0x4e0b90):

uint FUN_004e0b90(void)

{
  uint uVar1;
  short *psVar2;
  int in_EDX;
  int unaff_EDI;

  uVar1 = 0;
  psVar2 = (short *)(in_EDX + 0x3c4);
  do {
    if (*psVar2 == unaff_EDI) {
      uVar1 = uVar1 * 0x60 + 0x3b8 + in_EDX;
      *(undefined1 *)(uVar1 + 0x50) = 0;
      return uVar1 & 0xffffff00;
    }
    uVar1 = uVar1 + 1;
    psVar2 = psVar2 + 0x30;
  } while ((int)uVar1 < 0x10);
  DAT_00000050 = 0;
  return uVar1 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
