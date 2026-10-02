// player_profile_get_flag_by_id  (Ghidra: FUN_00495a60, unnamed)
// address 0x495a60, size 84 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x495a60..0x495ab3 (one-slot scan of 0x714dde, byte 0x712f0a + slot*0x2004).)
// evidence: out/phase4/interface_functions.md "Looks up a per-profile flag/byte value for the
// profile matching the given id."; reuses player_profile_find_index_by_id.c's single-slot
// profile_slot_id scan; the returned byte sits at profile_globals_block + slot*0x2004 + 0x132
// (0x00712f0a - 0x00712dd8), inside the same per-slot profile record player_profile_load.c
// copies in.
// register convention: profile id in EDX (in_EDX, low 16 bits significant).
// blam-cc: EDX -> id
// UNSURE: DAT_0071c2d8/DAT_0071c2d4 (a pair of flags that, when both zero, gate the id scan --
// otherwise the scan is skipped and the caller's own EDX value is reused as the "slot") are not
// documented anywhere; named locally by role only. The exact meaning of the returned byte
// (profile record + 0x132) is not recovered beyond "a per-profile flag".
// UNSURE: both return paths widen a single byte with decompiler CONCAT noise from an unrelated
// register; rewritten to return that byte directly (0 on the not-found path, matching the low
// byte the original's `uVar2 & 0xffffff00` mask leaves).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_client_globals *network_client; // 0x0071c2d8, as network_game_host_start.c (dword pointer)
extern network_server_globals *network_server; // 0x0071c2d4, as network_game_host_start.c (dword pointer)
extern int16_t profile_slot_id[];          // 0x00714dde (per player_profile_find_index_by_id.c)
extern uint8_t profile_globals_block[];  // 0x00712dd8 (per player_profile_subsystem_initialize.c)

// blam-cc: EDX -> id
// Finds the (sole, retail-PC) profile slot whose stored id matches `id` -- unless
// unknown_0071c2d8/unknown_0071c2d4 are not both clear, in which case the slot index defaults to
// `id` itself -- and returns the flag byte at offset 0x132 of that slot's profile record, or 0
// if no slot was found.
uint8_t player_profile_get_flag_by_id(int16_t id)
{
    int32_t slot;

    slot = id;
    if (network_client == (void *)0 && network_server == (void *)0) {
        slot = -1;
        if (profile_slot_id[0] == id) {
            slot = 0;
        }
    }
    if (slot != -1) {
        return profile_globals_block[slot * 0x2004 + 0x132];
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x495a60):

uint FUN_00495a60(void)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint in_EDX;

  uVar2 = in_EDX;
  if ((DAT_0071c2d8 == 0) && (DAT_0071c2d4 == 0)) {
    uVar1 = 0;
    do {
      uVar2 = uVar1;
      if (*(short *)(&DAT_00714dde + (short)uVar1 * 2) == (short)in_EDX) break;
      uVar1 = uVar1 + 1;
      uVar2 = 0xffffffff;
    } while ((short)uVar1 < 1);
  }
  if ((short)uVar2 != -1) {
    iVar3 = (short)uVar2 * 0x2004;
    return CONCAT31((int3)((uint)iVar3 >> 8),(&DAT_00712f0a)[iVar3]);
  }
  return uVar2 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
