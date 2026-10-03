// game_engine_ctf_initialize_flags  (Ghidra: game_engine_ctf_initialize_flags, already named)
// address 0x46d890, size 528 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: types/game.h ctf_globals (0x006b1290 live / 0x0087a520 replicated, flag_id_mask +0,
//   team_flag_id[16] +4); ctf_neutral_flag_id (0x006b1314); ctf_reset_ticks aliased 0x0087aa24;
//   custom_waypoint / custom_waypoints[32] (0x006f1888); game_variant::ctf_option_7c aliased
//   0x006f1d04; game_engine_ctf_assign_flag_ids (0x46d800, this batch); game_engine_ctf_pick_
//   random_flag (0x46dfe0, this batch).
// register convention: no parameters (__cdecl per Ghidra).
// UNSURE: the return value's upper 24 bits in the ctf_option_7c==2 path are Ghidra's own
//   CONCAT31 packing of a stray register with the real 1-byte result; narrowed to a plain int
//   here (matching the object_disconnect_from_map.c precedent for the same decompiler artifact).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario;    // 0x00746f8c
extern ctf_globals ctf_globals_live; // 0x006b1290
extern ctf_globals ctf_globals_network; // 0x0087a520
extern int32_t game_engine_ctf_reset_ticks; // 0x0087aa24
extern game_variant game_engine_variant;    // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)
extern int32_t ctf_neutral_flag_id;         // 0x006b1314
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern void game_engine_ctf_assign_flag_ids(void); // 0x46d800, this batch
extern int32_t game_engine_ctf_pick_random_flag(int32_t exclude_id); // 0x46dfe0, this batch
extern int16_t hud_waypoint_arrow_find(void); // 0x4af070, icon-name lookup

// Reassigns CTF flag usage ids (game_engine_ctf_assign_flag_ids), zeroes both the live and
// replicated ctf_globals blocks, resets the end-of-round timer, then walks every type-3
// ScenarioNetgameFlags entry: records its usage id in flag_id_mask, clears and re-registers the
// matching custom_waypoint slot at its position (raised 0.63 units), and tracks the lowest
// usage id seen. Depending on ctf_option_7c: 0 seeds every team_flag_id with that lowest id and
// returns 1; 2 picks a random flag as the neutral flag and returns 1; anything else clears every
// team_flag_id to -1 and returns -0xff.
int32_t game_engine_ctf_initialize_flags(void)
{
    int32_t lowest_usage_id = 0x20;
    int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
    int32_t i;

    game_engine_ctf_assign_flag_ids();

    {
        uint32_t *raw = (uint32_t *)&ctf_globals_live;
        for (i = 0; i < (int32_t)(sizeof(ctf_globals_live) / 4); i++) raw[i] = 0;
    }
    {
        uint32_t *raw = (uint32_t *)&ctf_globals_network;
        for (i = 0; i < (int32_t)(sizeof(ctf_globals_network) / 4); i++) raw[i] = 0;
    }
    game_engine_ctf_reset_ticks = 0x1e;

    for (i = 0; i < flag_count; i++) {
        ScenarioNetgameFlags *flag = &flags[i];
        int32_t usage_id;

        if (flag->type != 3 || flag->usage_id >= 0x20) {
            continue;
        }
        usage_id = flag->usage_id;
        if (usage_id < lowest_usage_id) {
            lowest_usage_id = usage_id;
        }
        ctf_globals_live.flag_id_mask |= 1u << (usage_id & 0x1f);

        {
            custom_waypoint *w = &custom_waypoints[usage_id];
            w->active = 0;
            w->icon = hud_waypoint_arrow_find();
            w->active = 1;
            w->position.x = flag->position.x;
            w->position.y = flag->position.y;
            w->position.z = flag->position.z;
            w->team = (int16_t)0xffff;
            w->owner = (datum_index)0xffffffff;
            w->position.z += 0.63f;
        }
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        ctf_neutral_flag_id = game_engine_ctf_pick_random_flag(-1);
        return 1;
    }
    if (game_engine_variant.engine.race.race_type != 0) {
        for (i = 0; i < 16; i++) {
            ctf_globals_live.team_flag_id[i] = -1;
        }
        return -0xff;
    }
    for (i = 0; i < 16; i++) {
        ctf_globals_live.team_flag_id[i] = lowest_usage_id;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x46d890), from tools/pack.py 0x46d890:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl game_engine_ctf_initialize_flags(void)

{
  short sVar1;
  int iVar2;
  undefined2 uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int local_c;
  int local_8;

  iVar2 = global_scenario;
  iVar5 = 0x20;
  FUN_0046d800();
  puVar6 = &DAT_006b1290;
  for (iVar4 = 0x52; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  puVar6 = &DAT_0087a520;
  for (iVar4 = 0x52; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  _DAT_0087aa24 = 0x1e;
  local_8 = 0;
  if (0 < *(int *)(iVar2 + 0x378)) {
    local_c = 0;
    do {
      puVar6 = (undefined4 *)(*(int *)(iVar2 + 0x37c) + local_c);
      if ((*(short *)(puVar6 + 4) == 3) && (sVar1 = *(short *)((int)puVar6 + 0x12), sVar1 < 0x20)) {
        if (sVar1 < iVar5) {
          iVar5 = (int)sVar1;
        }
        DAT_006b1290 = DAT_006b1290 | 1 << (*(byte *)((int)puVar6 + 0x12) & 0x1f);
        iVar4 = (int)*(short *)((int)puVar6 + 0x12);
        (&DAT_006f18a0)[iVar4 * 8] = 0xffffffff;
        uVar3 = FUN_004af070();
        *(undefined2 *)(&DAT_006f18a4 + iVar4 * 8) = uVar3;
        *(undefined1 *)(&DAT_006f1894 + iVar4 * 8) = 1;
        (&DAT_006f1888)[iVar4 * 8] = *puVar6;
        (&DAT_006f188c)[iVar4 * 8] = puVar6[1];
        (&DAT_006f1890)[iVar4 * 8] = puVar6[2];
        *(undefined2 *)(&DAT_006f189c + iVar4 * 8) = 0xffff;
        (&DAT_006f1898)[iVar4 * 8] = 0xffffffff;
        (&DAT_006f1890)[iVar4 * 8] = (float)(&DAT_006f1890)[iVar4 * 8] + 0.63;
      }
      local_8 = local_8 + 1;
      local_c = local_c + 0x94;
    } while (local_8 < *(int *)(iVar2 + 0x378));
  }
  if (_DAT_006f1d04 != 2) {
    if (_DAT_006f1d04 != 0) {
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
      return -0xff;
    }
    DAT_006b1294 = iVar5;
    DAT_006b1298 = iVar5;
    _DAT_006b129c = iVar5;
    _DAT_006b12a0 = iVar5;
    _DAT_006b12a4 = iVar5;
    _DAT_006b12a8 = iVar5;
    _DAT_006b12ac = iVar5;
    _DAT_006b12b0 = iVar5;
    _DAT_006b12b4 = iVar5;
    _DAT_006b12b8 = iVar5;
    _DAT_006b12bc = iVar5;
    _DAT_006b12c0 = iVar5;
    _DAT_006b12c4 = iVar5;
    _DAT_006b12c8 = iVar5;
    _DAT_006b12cc = iVar5;
    _DAT_006b12d0 = iVar5;
    return 1;
  }
  DAT_006b1314 = game_engine_ctf_pick_random_flag(-1);
  return CONCAT31((int3)((uint)DAT_006b1314 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
