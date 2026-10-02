// player_profile_find_index_by_id  (Ghidra: player_profile_find_index_by_id, already named)
// address 0x4954f0, size 33 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Finds the profile-slot index whose stored id
// matches the requested controller/profile id."; types/interface.h "global 0x00714dde:
// int16_t profile_slot_id[]  scanned by 0x4954f0" -- the loop's own bound ((short)i < 1 after
// the first increment) confirms retail PC has exactly one slot, matching the module's other
// single-local-player patterns.
// register convention: profile id in CX (in_CX). // blam-cc: CX -> id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t profile_slot_id[]; // 0x00714dde

// Returns 0 if `id` matches the (sole, retail-PC) profile slot's stored id, else -1.
// FIXED (objdump): every ret sets only AX; the upper bits of EAX are left as they were
int16_t player_profile_find_index_by_id(int16_t id)
{
    if (profile_slot_id[0] == id) {
        return 0;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4954f0):

int player_profile_find_index_by_id(void)

{
  int iVar1;
  short in_CX;

  iVar1 = 0;
  do {
    if (*(short *)(&DAT_00714dde + (short)iVar1 * 2) == in_CX) {
      return iVar1;
    }
    iVar1 = iVar1 + 1;
  } while ((short)iVar1 < 1);
  return CONCAT22((short)((uint)iVar1 >> 0x10),0xffff);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
