// game_engine_rate_player_starting_location  (Ghidra: FUN_00461d90)
// address 0x461d90, size 194 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// RENAMED by review. The first pass called this game_engine_compute_damage_scale (from
// out/phase4/game_functions.md's guess, "Computes the overall damage multiplier for a player by
// combining proximity, teammate-count, and game-variant scaling factors") and typed EAX as a
// damage_data *. Both are wrong: this is the player starting-location suitability scorer.
// evidence: VERIFIED against this function's ONE caller,
//   player_pick_random_starting_location (0x4776d0). At 0x477770 it does
//     mov eax,[esp+0x2c]  ; push eax          <- its own incoming player handle, the stack arg
//     mov eax,edi         ; call 0x461d90     <- EDI is the ScenarioPlayerStartingLocation *
//   and EDI is "scenario->player_starting_locations.pointer + i * 0x34". That pins EAX:
//   the +0x10 word this function compares against player::team (+0x20) is
//   ScenarioPlayerStartingLocation::team_index (types/tags.h, +0x10 in a 0x34-byte record), and
//   the same pointer is forwarded to 0x461ad0 / 0x461c60 as a real_point3d * because
//   ScenarioPlayerStartingLocation::position is that record's first field.
//   The two callees confirm it from the other side: 0x461ad0 penalizes a candidate point for
//   being within 0.25 / 1.0 / 2.0 / 5.0 world units of another player's unit and 0x461c60 pays
//   a bonus for same-team units 1.0..6.0 away -- Halo's classic spawn-point weighting, which has
//   nothing to do with damage.
//   types/game.h game_engine_definition::unknown_84 (+0x84, the teams-enabled predicate) and
//   ::unknown_70 (+0x70, an optional per-engine score override); game_variant::teams (+0x34,
//   aliased 0x006f1cbc).
// register convention: the candidate location in EAX (in_EAX), forwarded unchanged in ESI to
// both callees; the player handle is this function's own stack parameter (Ghidra declares it
// `float` only because the same stack slot is reused afterwards for the score accumulator).
//   // blam-cc: EAX -> location, stack -> player_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data;         // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant; // 0x006f1c88 (teams aliased 0x006f1cbc)

extern float game_engine_rate_location_crowding(uint32_t self_index, real_point3d *point); // 0x461ad0, this batch
extern float game_engine_rate_location_ally_bonus(uint32_t self_index, real_point3d *point); // 0x461c60, this batch

// blam-cc: EAX -> location, stack -> player_handle
// Scores one candidate player starting location for one player. In a team game whose engine
// says teams are on, a location reserved for a different team scores 0 outright; otherwise the
// score starts as the crowding penalty for the location's own position, is multiplied by the
// nearby-ally bonus when teams are on, and finally by the active engine's own optional score
// override if it installs one.
real game_engine_rate_player_starting_location(ScenarioPlayerStartingLocation *location,
    datum_index player_handle)
{
    player *p = (player *)((uint8_t *)player_data->data
                           + ((uint32_t)player_handle & 0xffff) * sizeof(player));
    real score;

    if (current_game_engine == 0 || current_game_engine->unknown_84 == 0 ||
        ((char (*)(int32_t))current_game_engine->unknown_84)(0) == 0 ||
        p->team == (int32_t)(int16_t)location->team_index) {
        score = game_engine_rate_location_crowding(player_handle,
                                                  (real_point3d *)&location->position);
    } else {
        score = 0.0f;
    }

    if (current_game_engine != 0) {
        if (0.0f < score && game_engine_variant.teams != 0) {
            score = game_engine_rate_location_ally_bonus(player_handle,
                                                        (real_point3d *)&location->position) * score;
        }
        if (current_game_engine->unknown_70 != 0) {
            // Called as "push esi ; push ebp ; call [engine+0x70]": cdecl, so the first
            // argument is EBP (the player handle) and the second is ESI (the location).
            real override_scale = ((real (*)(datum_index, ScenarioPlayerStartingLocation *))
                                   current_game_engine->unknown_70)(player_handle, location);
            return override_scale * score;
        }
    }
    return score;
}

#if 0
Original Ghidra decompilation (0x461d90), from tools/pack.py 0x461d90:

float10 FUN_00461d90(float param_1)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  int iVar3;
  float10 fVar4;

  uVar1 = (uint)param_1;
  iVar3 = *(int *)(DAT_0087a480 + 0x34);
  if ((((DAT_006f1d20 == 0) || (*(code **)(DAT_006f1d20 + 0x84) == (code *)0x0)) ||
      (cVar2 = (**(code **)(DAT_006f1d20 + 0x84))(0), cVar2 == '\0')) ||
     (*(int *)(((uint)param_1 & 0xffff) * 0x200 + iVar3 + 0x20) == (int)*(short *)(in_EAX + 0x10)))
  {
    iVar3 = DAT_006f1d20;
    fVar4 = (float10)FUN_00461ad0();
    param_1 = (float)fVar4;
  }
  else {
    param_1 = 0.0;
    iVar3 = DAT_006f1d20;
  }
  if (iVar3 != 0) {
    if ((0.0 < param_1) && (DAT_006f1cbc != '\0')) {
      fVar4 = (float10)FUN_00461c60();
      param_1 = (float)(fVar4 * (float10)param_1);
    }
    if (*(code **)(iVar3 + 0x70) != (code *)0x0) {
      fVar4 = (float10)(**(code **)(iVar3 + 0x70))(uVar1);
      return fVar4 * (float10)param_1;
    }
  }
  return (float10)param_1;
}
#endif
