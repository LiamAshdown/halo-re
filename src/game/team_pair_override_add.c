// team_pair_override_add  (Ghidra: FUN_0045be50; renamed per symbols/review_queue.txt)
// address 0x45be50, size 179 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED against objdump 0x45be50..0x45bf02; FIXED: team_pair_set gets BL = 0 (ally) -- the draft passed 1, so ai_allegiance made the teams enemies (marines attacked the player))
// evidence: types/game.h team_pair_override (every field offset below matches the struct
//   exactly: index_a_is_other 0x08, index_b 0x02, index_b_is_other 0x09, threshold 0x04, timer_reset 0x06,
//   other_is_human 0x0c, active 0x0a, status 0x0b).
// register convention: first index in EAX (in_AX); the rest are the recognized stack
//   parameters, in field-write order.
//   // blam-cc: EAX -> index_a, stack -> index_a_is_other, index_b, index_b_is_other, threshold,
//   //          timer_reset, other_is_human
//
// UNSURE: `team_pair_set(entry, 1, 0)` reconstructs FUN_0045c130(0)'s elided EAX/EBX arguments;
// `entry` is the slot just filled in (confident) and `active` = 1 matches the explicit
// `entry->active = 1` write immediately above it in the same block (best-effort, not
// independently verified -- see team_pair_set.c's own header).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern team_pair_globals *team_pair_data; // 0x006b0b84
extern void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary); // this batch, 0x45c130

// Finds an existing override for the (index_a, index_b) pair, or allocates a new slot (capped
// at k_maximum_team_pair_overrides), then (re)initializes its fields and activates it via
// team_pair_set.
void team_pair_override_add(int16_t index_a, uint8_t index_a_is_other, int16_t index_b, uint8_t index_b_is_other,
                             int16_t threshold, int16_t timer_reset, uint8_t other_is_human)
    // blam-cc: EAX -> index_a, stack -> index_a_is_other, index_b, index_b_is_other, threshold,
    //          timer_reset, other_is_human
{
    int16_t count;
    int16_t i;
    team_pair_override *entry;

    count = team_pair_data->override_count;
    i = 0;
    for (i = 0; i < count; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if ((entry->index_a == index_a && entry->index_b == index_b) ||
            (entry->index_b == index_a && entry->index_a == index_b)) {
            break;
        }
    }

    if (count <= i && count < 8) {
        team_pair_data->override_count = count + 1;
        i = count;
    }

    if (i < team_pair_data->override_count) {
        entry = &team_pair_data->overrides[i];
        entry->index_b_is_other = index_b_is_other;
        entry->index_a_is_other = index_a_is_other;
        entry->threshold = threshold;
        entry->index_a = index_a;
        entry->refcount = 0;
        entry->timer = 0;
        entry->index_b = index_b;
        entry->timer_reset = timer_reset;
        entry->other_is_human = other_is_human;
        entry->active = 1;
        // 0x45bee0..0x45befc: EAX = entry, BL = 0 (xor bl,bl), stack 0. BL = 0 SETS the pair's +0xa4 bits,
        // which teams_are_enemies reads as allied; BL = 1 (the betrayal path, 0x45c05f) clears them. The draft
        // passed 1, so (ai_allegiance player human) made the player and the marines enemies.
        team_pair_set(entry, 0, 0);
        entry->status = 0;
    }
}

#if 0
Original Ghidra decompilation (0x45be50), from tools/pack.py 0x45be50:

void FUN_0045be50(undefined1 param_1,short param_2,undefined1 param_3,short param_4,short param_5,
                 undefined1 param_6)

{
  short *psVar1;
  short sVar2;
  short in_AX;
  short sVar3;
  short *psVar4;

  psVar1 = DAT_006b0b84;
  sVar2 = *DAT_006b0b84;
  sVar3 = 0;
  psVar4 = DAT_006b0b84 + 1;
  if (0 < sVar2) {
    do {
      if (((*psVar4 == in_AX) && (psVar4[1] == param_2)) ||
         ((psVar4[1] == in_AX && (*psVar4 == param_2)))) break;
      sVar3 = sVar3 + 1;
      psVar4 = psVar4 + 9;
    } while (sVar3 < *DAT_006b0b84);
  }
  if ((sVar2 <= sVar3) && (sVar2 < 8)) {
    *DAT_006b0b84 = sVar2 + 1;
    sVar3 = sVar2;
  }
  if (sVar3 < *psVar1) {
    psVar1 = psVar1 + sVar3 * 9 + 1;
    *(undefined1 *)((int)psVar1 + 9) = param_3;
    *(undefined1 *)(psVar1 + 4) = param_1;
    psVar1[2] = param_4;
    *psVar1 = in_AX;
    psVar1[7] = 0;
    psVar1[8] = 0;
    psVar1[1] = param_2;
    psVar1[3] = param_5;
    *(undefined1 *)(psVar1 + 6) = param_6;
    *(undefined1 *)(psVar1 + 5) = 1;
    FUN_0045c130(0);
    *(undefined1 *)((int)psVar1 + 0xb) = 0;
  }
  return;
}
#endif
