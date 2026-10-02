// player_kill_streak_tick  (Ghidra: FUN_00479d10; renamed per the same kill-streak trio
// described in out/phase4/game_types_notes.md as 0x479ba0/0x479ca0/0x479d10)
// address 0x479d10, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x479d10..0x479d88 (EAX player; two int16 counters at player +0x68; slot 0 expiry clears unit +0x204 bit 0x10).)
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: types/game.h player::kill_streak[2] (0x68), player::unit (0x34); types/units.h
//   unit_flags::_unit_flag_unknown_10.
// register convention: a player index in EAX (in_EAX); no stack parameters.
//   // blam-cc: EAX -> player_index
// UNSURE: the `iVar6 == 0` guard means only slot 0's expiry clears the unit flag (slot 1's
//   expiry decrements silently); preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> player_index
// Decrements both of the player's kill_streak entries (floor 0), and when slot 0's countdown
// reaches exactly zero, clears unit_flags bit 0x10 on the player's unit.
void player_kill_streak_tick(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    int32_t slot;

    for (slot = 0; slot < 2; slot++) {
        if (0 < p->kill_streak[slot]) {
            p->kill_streak[slot] = p->kill_streak[slot] - 1;
            if (p->kill_streak[slot] == 0 && slot == 0) {
                unit_data *unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);
                unit->flags = unit->flags & ~_unit_flag_unknown_10;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x479d10), from tools/pack.py 0x479d10:

void FUN_00479d10(void)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  short *psVar7;
  int local_4;

  iVar3 = DAT_0087a480;
  iVar2 = DAT_008603b0;
  iVar5 = (in_EAX & 0xffff) * 0x200;
  psVar7 = (short *)(*(int *)(DAT_0087a480 + 0x34) + iVar5 + 0x68);
  local_4 = 2;
  iVar6 = 0;
  do {
    if (((0 < *psVar7) && (sVar4 = *psVar7 + -1, *psVar7 = sVar4, sVar4 == 0)) && (iVar6 == 0)) {
      puVar1 = (uint *)(*(int *)(*(int *)(iVar2 + 0x34) + 8 +
                                (*(uint *)(*(int *)(iVar3 + 0x34) + 0x34 + iVar5) & 0xffff) * 0xc) +
                       0x204);
      *puVar1 = *puVar1 & 0xffffffef;
    }
    iVar6 = iVar6 + 1;
    psVar7 = psVar7 + 1;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
