// game_engine_ctf_return_all_flags  (Ghidra: FUN_0046efe0; named per its summary)
// address 0x46efe0, size 438 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Re-initializes/returns all Capture-the-Flag flags
//   mid-round in team play, resetting their state and reassigning per-team flags similarly to
//   the full round-start initializer"); near-identical structure to game_engine_ctf_initialize_
//   flags.c (0x46d890, this batch), gated here on hosting (network_game_mode == 2) rather than
//   running unconditionally, and calling custom_waypoint_register (0x462260) per flag instead of
//   rebuilding the waypoint entirely.
// register convention: no parameters.
// UNSURE: custom_waypoint_register's own EAX/CX/EBX arguments (owner, slot, position) are
//   elided at this call site; only its three stack arguments (height_offset=0, player_filter=-1,
//   team_filter=-1) are visible, matching the sibling call in ctf_engine_flag_tick.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern int16_t network_game_mode; // 0x00719720
extern Scenario *global_scenario; // 0x00746f8c
extern ctf_globals ctf_globals_live; // 0x006b1290
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)
extern int32_t ctf_neutral_flag_id; // 0x006b1314

extern void game_engine_ctf_assign_flag_ids(void); // 0x46d800, this batch
extern int32_t game_engine_ctf_pick_random_flag(int32_t exclude_flag_index); // 0x46dfe0, this batch
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position,
    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260

void game_engine_ctf_return_all_flags(void)
{
    int32_t lowest_usage_id = 0x20;
    int32_t flag_count;
    ScenarioNetgameFlags *flags;
    int32_t i;

    if (network_game_mode != 2) {
        return;
    }

    game_engine_ctf_assign_flag_ids();
    {
        uint32_t *raw = (uint32_t *)&ctf_globals_live;
        for (i = 0; i < (int32_t)(sizeof(ctf_globals_live) / 4); i++) raw[i] = 0;
    }

    flag_count = (int32_t)global_scenario->netgame_flags.count;
    flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

    for (i = 0; i < flag_count; i++) {
        int32_t usage_id;
        if (flags[i].type != 3 || flags[i].usage_id >= 0x20) {
            continue;
        }
        usage_id = flags[i].usage_id;
        if (usage_id < lowest_usage_id) {
            lowest_usage_id = usage_id;
        }
        ctf_globals_live.flag_id_mask |= 1u << (usage_id & 0x1f);
        custom_waypoint_register((datum_index)0, (int16_t)0, (real_point3d *)0, 0.0f,
            (datum_index)0xffffffff, (int16_t)0xffffffff); // UNSURE forwarded owner/slot/position
    }

    if (game_engine_variant.ctf_option_7c == 2) {
        ctf_neutral_flag_id = game_engine_ctf_pick_random_flag(-1);
        return;
    }
    if (game_engine_variant.ctf_option_7c == 0) {
        for (i = 0; i < 16; i++) {
            ctf_globals_live.team_flag_id[i] = lowest_usage_id;
        }
        return;
    }
    for (i = 0; i < 16; i++) {
        ctf_globals_live.team_flag_id[i] = -1;
    }
}

#if 0
Original Ghidra decompilation (0x46efe0), from tools/pack.py 0x46efe0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046efe0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int local_8;
  int local_4;

  iVar2 = global_scenario;
  if (DAT_00719720 == 2) {
    iVar4 = 0x20;
    FUN_0046d800();
    puVar5 = &DAT_006b1290;
    for (iVar3 = 0x52; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    local_4 = 0;
    if (0 < *(int *)(iVar2 + 0x378)) {
      local_8 = 0;
      do {
        iVar3 = *(int *)(iVar2 + 0x37c) + local_8;
        if ((*(short *)(iVar3 + 0x10) == 3) && (sVar1 = *(short *)(iVar3 + 0x12), sVar1 < 0x20)) {
          if (sVar1 < iVar4) {
            iVar4 = (int)sVar1;
          }
          DAT_006b1290 = DAT_006b1290 | 1 << (*(byte *)(iVar3 + 0x12) & 0x1f);
          FUN_00462260(0,0xffffffff,0xffffffff);
        }
        local_4 = local_4 + 1;
        local_8 = local_8 + 0x94;
      } while (local_4 < *(int *)(iVar2 + 0x378));
    }
    if (_DAT_006f1d04 == 2) {
      DAT_006b1314 = game_engine_ctf_pick_random_flag(-1);
      return;
    }
    if (_DAT_006f1d04 == 0) {
      DAT_006b1294 = iVar4;
      DAT_006b1298 = iVar4;
      _DAT_006b129c = iVar4;
      _DAT_006b12a0 = iVar4;
      _DAT_006b12a4 = iVar4;
      _DAT_006b12a8 = iVar4;
      _DAT_006b12ac = iVar4;
      _DAT_006b12b0 = iVar4;
      _DAT_006b12b4 = iVar4;
      _DAT_006b12b8 = iVar4;
      _DAT_006b12bc = iVar4;
      _DAT_006b12c0 = iVar4;
      _DAT_006b12c4 = iVar4;
      _DAT_006b12c8 = iVar4;
      _DAT_006b12cc = iVar4;
      _DAT_006b12d0 = iVar4;
      return;
    }
    DAT_006b1294 = 0xffffffff;
    DAT_006b1298 = 0xffffffff;
    _DAT_006b129c = 0xffffffff;
    _DAT_006b12a0 = 0xffffffff;
    _DAT_006b12a4 = 0xffffffff;
    _DAT_006b12a8 = 0xffffffff;
    _DAT_006b12ac = 0xffffffff;
    _DAT_006b12b0 = 0xffffffff;
    _DAT_006b12b4 = 0xffffffff;
    _DAT_006b12b8 = 0xffffffff;
    _DAT_006b12bc = 0xffffffff;
    _DAT_006b12c0 = 0xffffffff;
    _DAT_006b12c4 = 0xffffffff;
    _DAT_006b12c8 = 0xffffffff;
    _DAT_006b12cc = 0xffffffff;
    _DAT_006b12d0 = 0xffffffff;
  }
  return;
}
#endif
