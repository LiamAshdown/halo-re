// player_advance_multikill_medal  (Ghidra: FUN_00479eb0; renamed per
// out/phase4/game_types_notes.md: "0x479eb0 -- 0xe0/0xe4 are the multikill medal counter and its
// window timer (thresholds at 0x006894a4 and 0x0069956c)")
// address 0x479eb0, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/game_types_notes.md; types/game.h player::medal_streak_count (0xe0),
//   player::medal_streak_timer (0xe4); types/memory.h data_array. The leading bounds/salt
//   check is the same manually inlined datum-validity test as player_add_kill_streak.c (this
//   batch).
// register convention: a player handle in ECX (in_ECX); no stack parameters.
//   // blam-cc: ECX -> player_handle
// UNSURE: network_session_autoban_player's effect (called with no visible arguments -- presumably fires the medal
//   event using the just-incremented counter via an elided register).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;              // 0x0087a480
extern int32_t multikill_medal_threshold;    // 0x006894a4
extern int32_t sv_tk_grace_ticks; // 0x0069956c
extern int32_t sv_tk_cooldown_ticks;   // 0x00699570

extern void network_session_autoban_player(void); // 0x4e36e0, not in this batch; UNSURE args elided

// blam-cc: ECX -> player_handle
// Validates `player_handle` exactly like player_add_kill_streak.c. If the player's
// medal_streak_timer hasn't expired (still >= 0), increments medal_streak_count; if that count
// reaches the configured threshold, fires the medal event (network_session_autoban_player) and returns without
// resetting the timer. Otherwise resets medal_streak_timer to -sv_tk_grace_ticks,
// falling back to sv_tk_cooldown_ticks if that negation is exactly zero.
void player_advance_multikill_medal(uint32_t player_handle)
{
    int16_t index;
    player *p;

    if (player_handle == 0xffffffff) {
        return;
    }
    index = (int16_t)player_handle;
    if (index < 0 || player_data->maximum_count <= index) {
        return;
    }

    p = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)index * player_data->size);
    if (p->identifier == 0) {
        return;
    }
    {
        int16_t salt = (int16_t)(player_handle >> 0x10);
        if (salt != 0 && p->identifier != salt) {
            return;
        }
    }

    if (p->medal_streak_timer < 0) {
        return;
    }

    p->medal_streak_count = p->medal_streak_count + 1;
    if (multikill_medal_threshold != 0 && multikill_medal_threshold <= p->medal_streak_count) {
        network_session_autoban_player();
        return;
    }

    p->medal_streak_timer = -sv_tk_grace_ticks;
    if (p->medal_streak_timer == 0) {
        p->medal_streak_timer = sv_tk_cooldown_ticks;
    }
}

#if 0
Original Ghidra decompilation (0x479eb0), from tools/pack.py 0x479eb0:

void FUN_00479eb0(void)

{
  int iVar1;
  short sVar2;
  int in_ECX;
  int iVar3;
  short sVar4;

  if (((in_ECX != -1) && (sVar2 = (short)in_ECX, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar1 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2;
    sVar2 = *(short *)(iVar1 + *(int *)(DAT_0087a480 + 0x34));
    iVar1 = iVar1 + *(int *)(DAT_0087a480 + 0x34);
    if (((sVar2 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar2 == sVar4))))
       && (-1 < *(int *)(iVar1 + 0xe4))) {
      iVar3 = *(int *)(iVar1 + 0xe0) + 1;
      *(int *)(iVar1 + 0xe0) = iVar3;
      if ((DAT_006894a4 != 0) && (DAT_006894a4 <= iVar3)) {
        FUN_004e36e0();
        return;
      }
      iVar3 = -DAT_0069956c;
      *(int *)(iVar1 + 0xe4) = iVar3;
      if (iVar3 == 0) {
        *(undefined4 *)(iVar1 + 0xe4) = DAT_00699570;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
