// team_pair_overrides_tick  (Ghidra: FUN_0045bcf0; renamed per symbols/review_queue.txt)
// address 0x45bcf0, size 91 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: types/game.h team_pair_override (timer 0x10, refcount 0x0e, timer_reset 0x06),
//   stride 0x12 (9 shorts) matching this loop exactly.
// register convention: no arguments.
//
// UNSURE: the call `team_pair_set(entry, 0, 0)` at the point an override's refcount reaches
// zero is transcribed from `FUN_0045c130(0)`, whose `entry` (EAX) and `active` (EBX) arguments
// are elided here; `entry` is confidently the override just ticked (matches every other call
// site's own local), `active` = 0 is a best-effort guess (deactivating the pair once its last
// countdown expires) and is not independently verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern team_pair_globals *team_pair_data; // 0x006b0b84
extern void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary); // this batch, 0x45c130

// Ticks every active override's countdown timer; once a countdown reaches zero, decrements the
// entry's refcount, and once THAT reaches zero, deactivates the pair -- otherwise the countdown
// is refreshed from the entry's own reset value.
void team_pair_overrides_tick(void)
{
    int16_t i;
    team_pair_override *entry;

    for (i = 0; i < team_pair_data->override_count; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if (0 < entry->timer) {
            entry->timer = entry->timer - 1;
            if (entry->timer == 0) {
                entry->refcount = entry->refcount - 1;
                if (entry->refcount == 0) {
                    team_pair_set(entry, 0, 0); // UNSURE: see header
                } else {
                    entry->timer = entry->timer_reset;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x45bcf0), from tools/pack.py 0x45bcf0:

void FUN_0045bcf0(void)

{
  short *psVar1;
  short sVar2;
  short *psVar3;
  short sVar4;

  psVar1 = DAT_006b0b84;
  sVar4 = 0;
  psVar3 = DAT_006b0b84 + 1;
  if (0 < *DAT_006b0b84) {
    do {
      if ((0 < psVar3[8]) && (sVar2 = psVar3[8] + -1, psVar3[8] = sVar2, sVar2 == 0)) {
        psVar3[7] = psVar3[7] + -1;
        if (psVar3[7] == 0) {
          FUN_0045c130(0);
        }
        else {
          psVar3[8] = psVar3[3];
        }
      }
      sVar4 = sVar4 + 1;
      psVar3 = psVar3 + 9;
    } while (sVar4 < *psVar1);
  }
  return;
}
#endif
