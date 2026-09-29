// actor_validate_grenade_ally_candidate  (Ghidra: actor_validate_grenade_ally_candidate, renamed)
// address 0x40e4a0, size 159 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x40e4a0..0x40e53e)
// evidence: phase-4 summary "validates a candidate actor (by handle) as eligible for a
// grenade-related interaction based on its current mode and team"; inlines the same
// index-and-salt validation datum_get performs against actor_data directly.
// register convention: candidate actor handle in ECX (Ghidra's in_ECX), a caller byte
// (compared against the candidate's per-type table byte) in BL (Ghidra's unaff_BL).
// blam-cc: ECX -> candidate_actor, BL -> caller_type_flag
// UNSURE: actor_type_procs[type][0x0c] is not named in types/ai.h (only +0x0d is, by a
// different function); kept as a raw offset via the same TYPES (folded into types/ai.h by the review pass) layout used in
// actor_update_melee_combat_action.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8

// TYPES (folded into types/ai.h by the review pass): see actor_update_melee_combat_action.c for the same layout.

// blam-cc: ECX -> candidate_actor, BL -> caller_type_flag
uint8_t actor_validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag)
{
    actor *candidate;
    int16_t index;
    int16_t salt;
    actor_type_table_entry *type_entry;

    candidate = (actor *)0;
    index = (int16_t)candidate_actor;
    if (candidate_actor != (datum_index)k_datum_index_none && -1 < index && index < actor_data->maximum_count) {
        actor *slot = (actor *)((uint8_t *)actor_data->data + (uint32_t)index * actor_data->size);
        salt = (int16_t)(candidate_actor >> 16);
        if (slot->identifier != 0 && (salt == 0 || slot->identifier == salt)) {
            candidate = slot;
        }
    }

    if (candidate != (actor *)0 && 1 < candidate->combat_status && candidate->combat_status < 4 &&
        (candidate->mode == 7 || candidate->mode == 5 ||
         (caller_type_flag == 0 && candidate->mode == 8) ||
         (candidate->mode == 6 && candidate->mode_data.raw[8] == 0 && 0 < *(int16_t *)&candidate->mode_data.raw[0]))) {
        type_entry = (actor_type_table_entry *)actor_type_procs[candidate->type];
        if (type_entry->unknown_0c != caller_type_flag) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40e4a0):

undefined4 FUN_0040e4a0(void)

{
  short *psVar1;
  undefined4 uVar2;
  short sVar3;
  int in_ECX;
  char unaff_BL;
  short sVar4;
  short *psVar5;

  psVar5 = (short *)0x0;
  if (((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_00880360 + 0x20))) {
    psVar1 = (short *)((int)*(short *)(DAT_00880360 + 0x22) * (int)sVar3 +
                      *(int *)(DAT_00880360 + 0x34));
    sVar3 = *psVar1;
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar3 == sVar4)))) {
      psVar5 = psVar1;
    }
  }
  uVar2 = 0;
  if (((((psVar5 != (short *)0x0) && (1 < psVar5[0x37])) && (psVar5[0x37] < 4)) &&
      (((sVar3 = psVar5[0x36], sVar3 == 7 || (sVar3 == 5)) ||
       (((unaff_BL == '\0' && (sVar3 == 8)) ||
        (((sVar3 == 6 && ((char)psVar5[0x52] == '\0')) && (0 < psVar5[0x4e])))))))) &&
     ((&PTR_PTR_006853b8)[psVar5[2]][0xc] != unaff_BL)) {
    uVar2 = 1;
  }
  return uVar2;
}
#endif
