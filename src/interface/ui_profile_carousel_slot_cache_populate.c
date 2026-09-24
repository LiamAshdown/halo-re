// ui_profile_carousel_slot_cache_populate  (Ghidra: FUN_004a74b0, renamed)
// address 0x4a74b0, size 185 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.75
// evidence: phase-4 summary "Assigns up to three profile pointers into the profile-carousel
// slot cache, validating each with player_profile_get". Phase-4 review against objdump
// 0x4a74b0..0x4a7568: the table is three profile_carousel_slot records of 0x2000 bytes
// (types/interface.h), so every slot index scales by 0x2000 (the earlier rewrite indexed a
// dword array, a pointer-stride error); the count is the one stack argument and the candidate
// id array arrives in EBX; player_profile_get gets the slot profile body in ECX and the id on
// the stack.
// UNSURE: when all three slots already hold other ids the free-slot scan ends at 3 and a
// successful load writes the id one record past the table, as in the binary.
// register convention: count on the stack, candidate ids in EBX.
//   // blam-cc: candidate_ids -> EBX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern profile_carousel_slot profile_carousel_slots[3]; // 0x00873d60
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770, blam-cc: ECX out_profile

// blam-cc: candidate_ids -> EBX
void ui_profile_carousel_slot_cache_populate(int32_t count, const int32_t *candidate_ids)
{
    uint8_t slot_kept[3] = { 0, 0, 0 };
    int32_t slot;
    int32_t i;

    for (slot = 0; slot < 3; slot++) {
        if (profile_carousel_slots[slot].profile_id != -1) {
            for (i = 0; i < count; i++) {
                if (profile_carousel_slots[slot].profile_id == candidate_ids[i]) {
                    slot_kept[slot] = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        int32_t id = candidate_ids[i];
        int32_t found;
        int32_t free_slot;

        if (id == -1) {
            continue;
        }
        for (found = 0; found < 3 && profile_carousel_slots[found].profile_id != id; found++) {
        }
        if (found != 3) {
            continue;
        }
        for (free_slot = 0; free_slot < 3 && slot_kept[free_slot] == 1; free_slot++) {
        }
        if (player_profile_get(id, profile_carousel_slots[free_slot].profile)) {
            profile_carousel_slots[free_slot].profile_id = candidate_ids[i];
            slot_kept[free_slot] = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a74b0):

void FUN_004a74b0(int param_1)

{
  char cVar1;
  int iVar2;
  uint in_ECX;
  int *piVar3;
  int unaff_EBX;
  int iVar4;
  int iVar5;
  uint local_4;

  iVar4 = 0;
  local_4 = in_ECX & 0xff000000;
  iVar5 = 0;
  piVar3 = &DAT_00873d60;
  do {
    if ((*piVar3 != -1) && (iVar2 = 0, 0 < param_1)) {
      do {
        if (*piVar3 == *(int *)(unaff_EBX + iVar2 * 4)) {
          *(undefined1 *)((int)&local_4 + iVar5) = 1;
          break;
        }
        iVar2 = iVar2 + 1;
      } while (iVar2 < param_1);
    }
    piVar3 = piVar3 + 0x800;
    iVar5 = iVar5 + 1;
  } while ((int)piVar3 < 0x879d60);
  if (0 < param_1) {
    do {
      iVar5 = *(int *)(unaff_EBX + iVar4 * 4);
      if (iVar5 != -1) {
        iVar2 = 0;
        piVar3 = &DAT_00873d60;
        do {
          if (iVar5 == *piVar3) break;
          piVar3 = piVar3 + 0x800;
          iVar2 = iVar2 + 1;
        } while ((int)piVar3 < 0x879d60);
        if (iVar2 == 3) {
          iVar2 = 0;
          do {
            if (*(char *)((int)&local_4 + iVar2) != '\x01') break;
            iVar2 = iVar2 + 1;
          } while (iVar2 < 3);
          cVar1 = player_profile_get(iVar5);
          if (cVar1 != '\0') {
            (&DAT_00873d60)[iVar2 * 0x800] = *(undefined4 *)(unaff_EBX + iVar4 * 4);
            *(undefined1 *)((int)&local_4 + iVar2) = 1;
          }
        }
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < param_1);
  }
  return;
}
#endif
