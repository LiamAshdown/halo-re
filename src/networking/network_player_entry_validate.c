// network_player_entry_validate  (Ghidra: FUN_004de9f0; named per this rewrite)
// address 0x4de9f0, size 138 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Register-based (EAX=player-slot pointer)
// validity check: confirms the slot's type/index fields are in range and its embedded
// wide-character name is properly NUL-terminated within its fixed-size field." entry+0x1d
// (machine_player_index, checked 0..0) and entry[0xe] as a short index (byte +0x1c,
// machine_index, checked 0..15) match types/networking.h's network_player_entry; the name scan
// walks entry->name[0..11] (0x00..0x17) looking for a NUL, matching name[12].
// register convention: entry in EAX (in_EAX). blam-cc: EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: EAX -> entry
// True when entry is non-NULL, its machine_player_index is 0, its machine_index is 0..15, and
// its name field contains a NUL within its first 12 UTF-16 code units.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
char network_player_entry_validate(network_player_entry *entry)
{
    int32_t i;

    if (entry != 0 && entry->machine_player_index >= 0 && entry->machine_player_index < 1 &&
        entry->machine_index >= 0 && entry->machine_index < 0x10) {
        for (i = 0; i < 12; i++) {
            if (entry->name[i] == 0) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de9f0):

uint FUN_004de9f0(void)

{
  short sVar1;
  short *in_EAX;
  short *psVar2;
  uint uVar3;

  if ((((in_EAX != (short *)0x0) && (-1 < *(char *)((int)in_EAX + 0x1d))) &&
      (*(char *)((int)in_EAX + 0x1d) < '\x01')) &&
     ((-1 < (char)in_EAX[0xe] && ((char)in_EAX[0xe] < '\x10')))) {
    uVar3 = 0;
    psVar2 = in_EAX;
    do {
      in_EAX = psVar2 + 1;
      if (*psVar2 == 0) {
LAB_004dea72:
        if (uVar3 < 0xc) {
          return CONCAT31((int3)((uint)in_EAX >> 8),1);
        }
        break;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 2;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 1;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 3;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 2;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 4;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 3;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 5;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 4;
        goto LAB_004dea72;
      }
      sVar1 = *in_EAX;
      in_EAX = psVar2 + 6;
      if (sVar1 == 0) {
        uVar3 = uVar3 + 5;
        goto LAB_004dea72;
      }
      uVar3 = uVar3 + 6;
      psVar2 = in_EAX;
    } while (uVar3 < 0xc);
  }
  return (uint)in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
