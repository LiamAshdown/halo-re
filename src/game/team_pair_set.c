// team_pair_set  (Ghidra: FUN_0045c130; renamed per symbols/review_queue.txt)
// address 0x45c130, size 400 bytes
// name confidence: 0.25   rewrite confidence: 0.85 (VERIFIED against objdump 0x45c130..0x45c2bf: BL = 0 sets the +0xa4 allied bits both ways, 1 clears them; the stack flag 0 sets / 1 clears the +0x94 bits; +0x0b = 1 and the AI is notified (a, b, BL, flag))
// evidence: types/game.h team_pair_override (index_a 0x00, index_b 0x02, active 0x0a, status
//   0x0b) and team_pair_globals (secondary_bits 0x94, enemy_bits 0xa4); updates both 10x10
//   bitmasks for a pair in both index orders.
// register convention: entry pointer in EAX (in_EAX); the new "active" state in EBX
//   (unaff_BL, also stored into the entry's active field); a secondary-bitmask gate as the
//   recognized stack parameter (param_1).
//   // blam-cc: EAX -> entry, EBX -> active, stack -> clear_secondary
//
// UNSURE: every one of this batch's four call sites invokes this with only the stack argument
// visible (`FUN_0045c130(0)` / `FUN_0045c130(1)`); `entry` and `active` are reconstructed at
// each call site rather than observed directly there. See each caller's own file for its
// reasoning.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern team_pair_globals *team_pair_data; // 0x006b0b84
extern void ai_notify_actors_of_encounter_state_change(int16_t team_a, int16_t team_b,
    uint8_t active, uint8_t clear_secondary); // 0x42b940

// Updates one pair's cached `active` flag and both 10x10 relationship bitmasks (secondary_bits
// governed by `clear_secondary`, enemy_bits governed by `active`), skipping the work entirely
// when neither would actually change anything.
extern void __cdecl standalone_log(const char *format, ...); // TEMPORARY play-test logging

void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary)
    // blam-cc: EAX -> entry, EBX -> active, stack -> clear_secondary
{
    standalone_log("DIAG team_pair_set a=%d b=%d BL=%d clear=%d entry_active=%d refcount=%d", entry->index_a,
        entry->index_b, active, clear_secondary, entry->active, entry->refcount); // TEMPORARY
    int32_t index;
    int32_t reverse_index;

    if (clear_secondary != 0 || entry->active != active) {
        entry->active = active;
        if (entry->index_a < 10 && entry->index_b < 10) {
            index = (int32_t)entry->index_b + entry->index_a * 10;
            reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
            if (clear_secondary == 0) {
                team_pair_data->secondary_bits[index >> 5] |= 1u << (index & 0x1f);
                team_pair_data->secondary_bits[reverse_index >> 5] |= 1u << (reverse_index & 0x1f);
            } else {
                team_pair_data->secondary_bits[index >> 5] &= ~(1u << (index & 0x1f));
                team_pair_data->secondary_bits[reverse_index >> 5] &= ~(1u << (reverse_index & 0x1f));
            }

            index = (int32_t)entry->index_b + entry->index_a * 10;
            if (active == 0) {
                team_pair_data->enemy_bits[index >> 5] |= 1u << (index & 0x1f);
                reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
                team_pair_data->enemy_bits[reverse_index >> 5] |= 1u << (reverse_index & 0x1f);
            } else {
                team_pair_data->enemy_bits[index >> 5] &= ~(1u << (index & 0x1f));
                reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
                team_pair_data->enemy_bits[reverse_index >> 5] &= ~(1u << (reverse_index & 0x1f));
            }
        }
        entry->status = 1;
        ai_notify_actors_of_encounter_state_change(entry->index_a, entry->index_b, active, clear_secondary);
    }
}

#if 0
Original Ghidra decompilation (0x45c130), from tools/pack.py 0x45c130:

void FUN_0045c130(char param_1)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  short *in_EAX;
  uint uVar4;
  uint *puVar5;
  char unaff_BL;
  int iVar6;
  uint uVar7;

  if ((param_1 != '\0') || ((char)in_EAX[5] != unaff_BL)) {
    *(char *)(in_EAX + 5) = unaff_BL;
    iVar3 = DAT_006b0b84;
    if ((*in_EAX < 10) && (in_EAX[1] < 10)) {
      iVar1 = (int)in_EAX[1] + *in_EAX * 10;
      iVar6 = iVar1 >> 5;
      bVar2 = (byte)iVar1;
      if (param_1 == '\0') {
        *(uint *)(DAT_006b0b84 + 0x94 + iVar6 * 4) =
             *(uint *)(DAT_006b0b84 + 0x94 + iVar6 * 4) | 1 << (bVar2 & 0x1f);
        iVar1 = (int)*in_EAX + in_EAX[1] * 10;
        iVar6 = iVar1 >> 5;
        puVar5 = (uint *)(iVar3 + 0x94 + iVar6 * 4);
        uVar4 = *(uint *)(iVar3 + 0x94 + iVar6 * 4) | 1 << ((byte)iVar1 & 0x1f);
      }
      else {
        *(uint *)(DAT_006b0b84 + 0x94 + iVar6 * 4) =
             *(uint *)(DAT_006b0b84 + 0x94 + iVar6 * 4) & ~(1 << (bVar2 & 0x1f));
        iVar1 = (int)*in_EAX + in_EAX[1] * 10;
        iVar6 = iVar1 >> 5;
        puVar5 = (uint *)(iVar3 + 0x94 + iVar6 * 4);
        uVar4 = *(uint *)(iVar3 + 0x94 + iVar6 * 4) & ~(1 << ((byte)iVar1 & 0x1f));
      }
      *puVar5 = uVar4;
      iVar1 = (int)in_EAX[1] + *in_EAX * 10;
      iVar6 = iVar1 >> 5;
      uVar7 = 1 << ((byte)iVar1 & 0x1f);
      uVar4 = *(uint *)(iVar3 + 0xa4 + iVar6 * 4);
      puVar5 = (uint *)(iVar3 + 0xa4 + iVar6 * 4);
      if (unaff_BL == '\0') {
        *puVar5 = uVar4 | uVar7;
        iVar1 = (int)*in_EAX + in_EAX[1] * 10;
        puVar5 = (uint *)(iVar3 + 0xa4 + (iVar1 >> 5) * 4);
        uVar4 = *puVar5 | 1 << ((byte)iVar1 & 0x1f);
      }
      else {
        *puVar5 = uVar4 & ~uVar7;
        iVar1 = (int)*in_EAX + in_EAX[1] * 10;
        puVar5 = (uint *)(iVar3 + 0xa4 + (iVar1 >> 5) * 4);
        uVar4 = *puVar5 & ~(1 << ((byte)iVar1 & 0x1f));
      }
      *puVar5 = uVar4;
    }
    *(undefined1 *)((int)in_EAX + 0xb) = 1;
    ai_notify_actors_of_encounter_state_change(*in_EAX,in_EAX[1],unaff_BL,param_1);
  }
  return;
}
#endif
