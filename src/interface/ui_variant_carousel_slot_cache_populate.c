// ui_variant_carousel_slot_cache_populate  (Ghidra: FUN_004a7570, renamed)
// address 0x4a7570, size 189 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.8
// phase-4 review: checked against objdump 0x4a7570..0x4a762c. The count is the one stack
// argument ([esp+0x14] after four pushes), not EAX; ECX is only pushed as scratch space. The
// UNSURE free_slot == 3 case below is real in the binary (it would pass 0x00879f38).
// evidence: phase-4 summary "Assigns up to three game-variant entries into the
// variant-selection carousel slot cache"; the 0x9c byte stride over exactly 3 slots
// (0x00879d60..0x00879f34) sits immediately after the profile carousel cache's 3 slots
// (0x00873d60..0x00879d60, see ui_profile_carousel_slot_cache_populate.c), which is why both
// tables share the address 0x00879d60 as one's end bound and the other's start.
// register convention: candidate id array in EBX; the count on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern variant_carousel_slot variant_carousel_slots[3]; // 0x00879d60
extern uint8_t saved_game_get_variant(int32_t variant_id, void *out_slot_body); // 0x53bee0, UNSURE signature, not in this module's range

// Marks which of the 3 game-variant carousel slots already hold one of the candidate ids, then
// assigns each still-unmatched candidate into the first free slot once saved_game_get_variant fills in
// and confirms the variant data.
// blam-cc: EBX -> candidate_ids, stack -> count
void ui_variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count)
{
    uint8_t slot_filled[3] = {0, 0, 0};
    int32_t slot;
    int32_t i;

    for (slot = 0; slot < 3; slot = slot + 1) {
        if (variant_carousel_slots[slot].id != -1) {
            for (i = 0; i < count; i = i + 1) {
                if (variant_carousel_slots[slot].id == candidate_ids[i]) {
                    slot_filled[slot] = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < count; i = i + 1) {
        int32_t id = candidate_ids[i];
        if (id != -1) {
            int32_t found_slot = 0;
            while (found_slot < 3 && id != variant_carousel_slots[found_slot].id) {
                found_slot = found_slot + 1;
            }
            if (found_slot == 3) {
                int32_t free_slot = 0;
                while (free_slot < 3 && slot_filled[free_slot] == 1) {
                    free_slot = free_slot + 1;
                }
                if (saved_game_get_variant(id, variant_carousel_slots[free_slot].unknown)) { // UNSURE: free_slot can be 3
                    variant_carousel_slots[free_slot].id = id;
                    slot_filled[free_slot] = 1;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a7570):

void FUN_004a7570(int param_1)

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
  piVar3 = &DAT_00879d60;
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
    piVar3 = piVar3 + 0x27;
    iVar5 = iVar5 + 1;
  } while ((int)piVar3 < 0x879f34);
  if (0 < param_1) {
    do {
      iVar5 = *(int *)(unaff_EBX + iVar4 * 4);
      if (iVar5 != -1) {
        iVar2 = 0;
        piVar3 = &DAT_00879d60;
        do {
          if (iVar5 == *piVar3) break;
          piVar3 = piVar3 + 0x27;
          iVar2 = iVar2 + 1;
        } while ((int)piVar3 < 0x879f34);
        if (iVar2 == 3) {
          iVar2 = 0;
          do {
            if (*(char *)((int)&local_4 + iVar2) != '\x01') break;
            iVar2 = iVar2 + 1;
          } while (iVar2 < 3);
          cVar1 = FUN_0053bee0(iVar5,&DAT_00879d64 + iVar2 * 0x27);
          if (cVar1 != '\0') {
            (&DAT_00879d60)[iVar2 * 0x27] = *(undefined4 *)(unaff_EBX + iVar4 * 4);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
