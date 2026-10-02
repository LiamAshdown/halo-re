// player_pick_random_starting_location  (Ghidra: FUN_004776d0; named per
// out/phase4/game_functions.md: "Picks a random-weighted player starting location index for a
// respawn, biased toward locations with a higher suitability score.")
// address 0x4776d0, size 312 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x4776d0
//   --stop-address=0x4777f8): the caller is player_respawn (0x477ea0), which saves its own
//   player-handle argument into EBX before clobbering EAX and pushes that EBX as this
//   function's one stack parameter -- so `param_1` is the player handle, matched into
//   game_engine_rate_player_starting_location (0x461d90)'s own stack parameter. The location count
//   and pointer are Scenario::player_starting_locations (types/tags.h TagReflexive, at
//   scenario+0x354/+0x358); each ScenarioPlayerStartingLocation's game-type list is `type_0`
//   (+0x14, a 4-entry array, confirmed against netgame_equipment_game_type_matches's own
//   `count=4` convention). netgame_equipment_game_type_matches (0x45f7c0),
//   game_engine_location_blocked_by_vehicle (0x461e60) and
//   game_engine_rate_player_starting_location (0x461d90) are siblings in this module.
//   CORRECTED by review: the first pass called both of those with Ghidra's own (empty and
//   float) argument lists. The disassembly shows the candidate location pointer live in a
//   register at each call -- "mov edx,edi ; call 0x461e60" and "mov eax,edi ; call 0x461d90",
//   with EDI = locations.pointer + i * 0x34 -- so both now receive `location` explicitly, and
//   the scorer additionally receives the player handle as its stack argument (the "mov
//   eax,[esp+0x2c] ; push eax" immediately before it).
//   The RNG step is the engine-wide LCG (random_seed_global, 0x00719cd0, seed = seed *
//   0x19660d + 0x3c6ef35f) turned into a 0..1 float by multiplying the top 16 bits by
//   DAT_00672b84 (the same 1/65536-family scale periodic_function_build_transition_table's
//   case-0 constant family uses), then raised to the power DAT_00672cf0 == 0.5 (sqrt) via the
//   MSVC7.1 CRT `_CIpow` at 0x6283c0 (base in ST(1), exponent in ST(0), per
//   periodic_function_build_transition_table.c's own note on that call).
// register convention: none (this function's argument, the respawning player's index, is its
//   own single stack parameter; nothing arrives in a register).
//   // blam-cc: stack -> player_index
// UNSURE: DAT_00672b84's exact float32 bit pattern was not re-derived here (kept as a named
//   extern rather than a literal); it is presumed to be the 1/65536 family constant used
//   elsewhere for "top 16 LCG bits -> 0..1 float".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario;      // 0x00746f8c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern random_seed random_seed_global;    // 0x00719cd0
extern float k_random_scale_65536; // 0x00672b84 = 1.5259022e-05f (VERIFIED against disassembly 0x4777a4..0x4777a8, 2026-09-30)
extern double sqrt_pow_exponent;       // 0x00672cf0 QWORD == 0.5

extern uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count,
    int32_t current_engine_index); // 0x45f7c0
extern uint8_t game_engine_location_blocked_by_vehicle(real_point3d *point); // 0x461e60, this
    // module; blam-cc: EDX -> point
extern real game_engine_rate_player_starting_location(ScenarioPlayerStartingLocation *location,
    datum_index player_handle); // 0x461d90, this module; blam-cc: EAX -> location,
    // stack -> player_handle
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow

// blam-cc: stack -> player_index
// Walks Scenario::player_starting_locations, scoring every entry whose game-type list matches
// the active game engine (or, outside multiplayer, whose list is entirely wildcard/zero) and
// that has no vehicle parked on it, by sqrt(random 0..1) * suitability, and returns the index of
// the highest-scoring entry, or -1 if the scenario has no starting locations at all (or if every
// candidate scored 0, since best_index starts at -1 and the comparison is strict).
// NOTE: the original guards the per-iteration location pointer with "if (i >= 0 && i < count)"
// and passes NULL when that fails, which cannot happen for i in [0, count); the do/while below
// drops the dead guard and walks the array directly.
int16_t player_pick_random_starting_location(datum_index player_handle)
{
    int16_t count;
    int16_t index;
    int16_t best_index;
    float best_score;
    ScenarioPlayerStartingLocation *location;
    int32_t current_engine_index;

    count = (int16_t)global_scenario->player_starting_locations.count;
    index = 0;
    best_index = -1;
    best_score = 0.0f;

    if (0 < count) {
        location = (ScenarioPlayerStartingLocation *)global_scenario->player_starting_locations.pointer;
        do {
            float suitability;
            uint8_t matches;

            current_engine_index = (current_game_engine != 0) ? current_game_engine->index : -1;
            matches = netgame_equipment_game_type_matches(&location->type_0, 4, current_engine_index);
            if (!matches) {
                suitability = 0.0f;
            } else if (game_engine_location_blocked_by_vehicle(
                           (real_point3d *)&location->position) != 0) {
                suitability = 0.0f;
            } else {
                suitability = game_engine_rate_player_starting_location(location, player_handle);
            }

            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            {
                float unit_random = (float)(random_seed_global >> 0x10) * k_random_scale_65536;
                float weight = (float)pow((double)unit_random, sqrt_pow_exponent) * suitability;
                if (best_score < weight) {
                    best_score = weight;
                    best_index = index;
                }
            }

            index = index + 1;
            location = location + 1;
        } while (index < count);
        return best_index;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4776d0), from tools/pack.py 0x4776d0:

int FUN_004776d0(undefined4 param_1)

{
  short sVar1;
  short sVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  float10 fVar6;
  float local_18;
  int local_14;
  int local_10;
  float local_c;

  sVar1 = *(short *)(global_scenario + 0x354);
  sVar4 = 0;
  sVar2 = -1;
  local_c = 0.0;
  if (0 < sVar1) {
    local_14 = 0;
    local_10 = 0;
    do {
      iVar5 = 0;
      if ((-1 < sVar4) && (local_14 < *(int *)(global_scenario + 0x354))) {
        iVar5 = *(int *)(global_scenario + 0x358) + local_10;
      }
      cVar3 = FUN_0045f7c0(iVar5 + 0x14);
      if (cVar3 == '\0') {
        local_18 = 0.0;
      }
      else {
        cVar3 = FUN_00461e60();
        if (cVar3 == '\0') {
          fVar6 = (float10)FUN_00461d90(param_1);
          local_18 = (float)fVar6;
        }
        else {
          local_18 = 0.0;
        }
      }
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      fVar6 = (float10)FUN_006283c0();
      if ((float10)local_c < fVar6 * (float10)local_18) {
        local_c = (float)(fVar6 * (float10)local_18);
        sVar2 = sVar4;
      }
      sVar4 = sVar4 + 1;
      local_14 = local_14 + 1;
      local_10 = local_10 + 0x34;
    } while (sVar4 < sVar1);
    return (int)sVar2;
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
