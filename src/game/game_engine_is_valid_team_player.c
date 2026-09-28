// game_engine_is_valid_team_player  (Ghidra: FUN_00466b60; named per
// out/phase4/game_functions.md: "Predicate checking whether a given identifier refers to a
// usable, team-assigned player under current game-engine constraints.")
// address 0x466b60, size 94 bytes
// name confidence: 0.3   rewrite confidence: 0.65
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466b60
//   --stop-address=0x466bc0): `identifier` is a plain stack parameter (not a register), matching
//   Ghidra's own recognized `param_1`. types/game.h game_engine_unknown_aa00 (0x0087aa00) and
//   player::local_player_index (+0x02); light_count_enabled (0x0068944c) is not owned by this module
//   either (see src/objects/lights_apply_spot_falloff.c, which keeps the same UNSURE name).
//   player_index_from_unit_index (0x474db0) is outside this batch; out/phase4/game_functions.md notes its own
//   decompiled view always returns -1, so its real per-call behavior is not established here.
// register convention: `identifier` is this function's first (and only) stack parameter.
// UNSURE: player_index_from_unit_index's real name/signature/behavior; g_0068944c's owning module and meaning.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t game_engine_unknown_aa00; // 0x0087aa00
extern int32_t light_count_enabled;               // 0x0068944c, UNSURE: not owned by this module
extern data_array *player_data;          // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern datum_index player_index_from_unit_index(datum_index object_or_unit); // 0x474db0, UNSURE signature/behavior

// Returns whether `identifier` refers to a player that currently has a real team assignment.
// True unconditionally outside a multiplayer engine; true unconditionally once game_engine_
// unknown_aa00 bit 1 is set or fewer than 2 teams are in play; otherwise resolves `identifier`
// through player_index_from_unit_index and checks that the resulting player's local_player_index (+0x02) is set.
uint8_t game_engine_is_valid_team_player(uint32_t identifier)
{
    uint32_t player_index;
    player *p;

    if (current_game_engine == 0) {
        return 1;
    }
    if ((game_engine_unknown_aa00 & 2) != 0) {
        return 0;
    }
    if (light_count_enabled < 1) {
        return 0;
    }
    if (light_count_enabled >= 2) {
        return 1;
    }

    player_index = player_index_from_unit_index(identifier);
    if (player_index == 0xffffffff) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    return p->local_player_index != -1;
}

#if 0
Original Ghidra decompilation (0x466b60), from tools/pack.py 0x466b60:

bool FUN_00466b60(undefined4 param_1)

{
  bool bVar1;
  uint uVar2;

  bVar1 = true;
  if (DAT_006f1d20 != 0) {
    if (((DAT_0087aa00 & 2) != 0) || (DAT_0068944c < 1)) {
      return false;
    }
    bVar1 = true;
    if (DAT_0068944c < 2) {
      uVar2 = FUN_00474db0(param_1);
      if (uVar2 != 0xffffffff) {
        return *(short *)((uVar2 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1;
      }
      bVar1 = false;
    }
  }
  return bVar1;
}
#endif
