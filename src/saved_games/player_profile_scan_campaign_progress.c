// player_profile_scan_campaign_progress  (Ghidra: FUN_00539e00, renamed)
// address 0x539e00, size 482 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: out/phase4/saved_games_functions.md summary "Scans a table of per-slot byte flags
// to find the last (highest-priority) active entry and its type, purpose not fully certain from
// code alone." The compiler unrolled a plain 10-byte loop into two 5-wide groups with
// convoluted pointer offsets that all cancel out to `campaign_progress[i]`; rewritten as the
// equivalent 10-iteration loop (out/phase4/saved_games_types_notes.md's
// saved_player_profile::campaign_progress note: "per level, bit n set = finished on difficulty
// n"). It walks every level in order and, for any level with a nonzero byte, records that
// level's index and the *highest* difficulty bit set (bit 3 checked before 2, 1, 0) -- so the
// last call with a nonzero byte (the highest level index reached) wins, giving the highest
// level completed and the best difficulty it was completed on. If a byte were nonzero but had
// none of bits 0..3 set, neither output would be touched for that level -- reproduced exactly,
// not "fixed", since campaign_progress bytes are documented as only ever using those 4 bits.
// register convention: out_type in ECX, profile in EDX, out_level in ESI (profile base in EDX
// matches the module's register-convention note for this and 0x539ff0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// blam-cc: out_type in ECX, profile in EDX, out_level in ESI
void player_profile_scan_campaign_progress(int16_t *out_type, saved_player_profile *profile, int16_t *out_level)
{
    int32_t i;
    uint8_t progress;

    *out_level = -1;
    *out_type = 1;

    for (i = 0; i < k_campaign_level_count; i = i + 1) {
        progress = profile->campaign_progress[i];
        if (progress != 0) {
            if ((progress & 8) != 0) {
                *out_level = (int16_t)i;
                *out_type = 3;
            } else if ((progress & 4) != 0) {
                *out_level = (int16_t)i;
                *out_type = 2;
            } else if ((progress & 2) != 0) {
                *out_level = (int16_t)i;
                *out_type = 1;
            } else if ((progress & 1) != 0) {
                *out_level = (int16_t)i;
                *out_type = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x539e00):

void FUN_00539e00(void)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  undefined2 *in_ECX;
  int in_EDX;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short *unaff_ESI;
  byte *pbVar8;

  pbVar8 = (byte *)(in_EDX + 0x11f);
  iVar6 = in_EDX - (int)pbVar8;
  iVar7 = in_EDX - (int)pbVar8;
  iVar5 = in_EDX - (int)pbVar8;
  iVar4 = in_EDX - (int)pbVar8;
  iVar3 = 0;
  *unaff_ESI = -1;
  *in_ECX = 1;
  do {
    bVar1 = pbVar8[iVar5 + 0x11e];
    sVar2 = (short)iVar3;
    if (bVar1 != 0) {
      if ((bVar1 & 8) == 0) {
        if ((bVar1 & 4) == 0) {
          if ((bVar1 & 2) == 0) {
            if ((bVar1 & 1) != 0) {
              *unaff_ESI = sVar2;
              *in_ECX = 0;
            }
          }
          else {
            *unaff_ESI = sVar2;
            *in_ECX = 1;
          }
        }
        else {
          *unaff_ESI = sVar2;
          *in_ECX = 2;
        }
      }
      else {
        *unaff_ESI = sVar2;
        *in_ECX = 3;
      }
    }
    bVar1 = *pbVar8;
    if (bVar1 != 0) {
      if ((bVar1 & 8) == 0) {
        if ((bVar1 & 4) == 0) {
          if ((bVar1 & 2) == 0) {
            if ((bVar1 & 1) != 0) {
              *unaff_ESI = sVar2 + 1;
              *in_ECX = 0;
            }
          }
          else {
            *unaff_ESI = sVar2 + 1;
            *in_ECX = 1;
          }
        }
        else {
          *unaff_ESI = sVar2 + 1;
          *in_ECX = 2;
        }
      }
      else {
        *unaff_ESI = sVar2 + 1;
        *in_ECX = 3;
      }
    }
    bVar1 = pbVar8[iVar6 + 0x120];
    if (bVar1 != 0) {
      if ((bVar1 & 8) == 0) {
        if ((bVar1 & 4) == 0) {
          if ((bVar1 & 2) == 0) {
            if ((bVar1 & 1) != 0) {
              *unaff_ESI = sVar2 + 2;
              *in_ECX = 0;
            }
          }
          else {
            *unaff_ESI = sVar2 + 2;
            *in_ECX = 1;
          }
        }
        else {
          *unaff_ESI = sVar2 + 2;
          *in_ECX = 2;
        }
      }
      else {
        *unaff_ESI = sVar2 + 2;
        *in_ECX = 3;
      }
    }
    bVar1 = pbVar8[iVar7 + 0x121];
    if (bVar1 != 0) {
      if ((bVar1 & 8) == 0) {
        if ((bVar1 & 4) == 0) {
          if ((bVar1 & 2) == 0) {
            if ((bVar1 & 1) != 0) {
              *unaff_ESI = sVar2 + 3;
              *in_ECX = 0;
            }
          }
          else {
            *unaff_ESI = sVar2 + 3;
            *in_ECX = 1;
          }
        }
        else {
          *unaff_ESI = sVar2 + 3;
          *in_ECX = 2;
        }
      }
      else {
        *unaff_ESI = sVar2 + 3;
        *in_ECX = 3;
      }
    }
    bVar1 = pbVar8[iVar4 + 0x122];
    if (bVar1 != 0) {
      if ((bVar1 & 8) == 0) {
        if ((bVar1 & 4) == 0) {
          if ((bVar1 & 2) == 0) {
            if ((bVar1 & 1) != 0) {
              *unaff_ESI = sVar2 + 4;
              *in_ECX = 0;
            }
          }
          else {
            *unaff_ESI = sVar2 + 4;
            *in_ECX = 1;
          }
        }
        else {
          *unaff_ESI = sVar2 + 4;
          *in_ECX = 2;
        }
      }
      else {
        *unaff_ESI = sVar2 + 4;
        *in_ECX = 3;
      }
    }
    iVar3 = iVar3 + 5;
    pbVar8 = pbVar8 + 5;
  } while (iVar3 < 10);
  return;
}
#endif
