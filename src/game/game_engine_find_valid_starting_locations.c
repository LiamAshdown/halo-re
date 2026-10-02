// game_engine_find_valid_starting_locations  (Ghidra: game_engine_find_valid_starting_locations,
// already named -- but see the UNSURE note below, which types/game.h's misattribution list
// (out/phase4/game_types_notes.md item 3) already flags)
// address 0x461080, size 249 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: Ghidra's own __cdecl signature recovery (all six parameters, no hidden registers);
// types/tags.h ScenarioNetgameFlags (position, facing, type +0x10, usage_id +0x12), Scenario
// (netgame_flags TagReflexive, count/pointer at the same +0x378/+0x37c this function reads).
// UNSURE (per out/phase4/game_types_notes.md item 3): despite its name and its `team`/`type`
// parameter names, this function walks `Scenario::netgame_flags`, NOT
// `player_starting_locations` -- and its two short parameters are compared against
// `ScenarioNetgameFlags::type` (+0x10) and `::usage_id` (+0x12) respectively, i.e. swapped
// relative to what the names suggest. Kept exactly as Ghidra names them (to match every existing
// caller's positional argument order) with that swap called out here and at each comparison.

#include "tags.h"
#include "cache.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c

// UNSURE: `team` is actually compared to ScenarioNetgameFlags::type, and `type` to
// ScenarioNetgameFlags::usage_id -- see the file header. `origin` arrives via EBX
// (unaff_EBX) and is optional (NULL disables the distance/height filtering entirely).
//   // blam-cc: unaff_EBX -> origin, stack -> max_horizontal_dist, max_height_delta, team, type,
//   //          max_results, results
int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist,
    float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results)
{
    int32_t found = 0;
    int16_t i;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

    for (i = 0; i < (int32_t)global_scenario->netgame_flags.count; i++) {
        ScenarioNetgameFlags *f = &flags[i];

        if ((team == -1 || team == (int16_t)f->type) &&
            (type == -1 || type == (int16_t)f->usage_id) &&
            (origin == 0 ||
             ((max_horizontal_dist < 0.0f ||
               (origin->y - f->position.y) * (origin->y - f->position.y) +
               (origin->z - f->position.z) * (origin->z - f->position.z) +
               (origin->x - f->position.x) * (origin->x - f->position.x) <=
               max_horizontal_dist * max_horizontal_dist) &&
              (max_height_delta <= 0.0f ||
               (((f->position.z - origin->z) < 0.0f) ? -(f->position.z - origin->z)
                : (f->position.z - origin->z)) <= max_height_delta)))) {
            if (found < max_results) {
                results[found] = i;
                found = found + 1;
            }
        }
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x461080), from tools/pack.py 0x461080:

int __cdecl
game_engine_find_valid_starting_locations
          (float max_horizontal_dist_sq,float max_height_delta,short team,short type,int max_results
          ,int *results)

{
  int iVar1;
  float *pfVar2;
  int iVar3;
  float *unaff_EBX;
  short sVar4;
  int iVar5;
  
  iVar1 = global_scenario;
  iVar5 = 0;
  sVar4 = 0;
  if (0 < *(int *)(global_scenario + 0x378)) {
    iVar3 = 0;
    do {
      pfVar2 = (float *)(iVar3 * 0x94 + *(int *)(iVar1 + 0x37c));
      if (((((team == -1) || (team == *(short *)(pfVar2 + 4))) &&
           ((type == -1 || (type == *(short *)((int)pfVar2 + 0x12))))) &&
          ((unaff_EBX == (float *)0x0 ||
           (((max_horizontal_dist_sq < 0.0 ||
             ((unaff_EBX[1] - pfVar2[1]) * (unaff_EBX[1] - pfVar2[1]) +
              (unaff_EBX[2] - pfVar2[2]) * (unaff_EBX[2] - pfVar2[2]) +
              (*unaff_EBX - *pfVar2) * (*unaff_EBX - *pfVar2) <=
              max_horizontal_dist_sq * max_horizontal_dist_sq)) &&
            ((max_height_delta <= 0.0 || (ABS(pfVar2[2] - unaff_EBX[2]) <= max_height_delta))))))))
         && (iVar5 < max_results)) {
        results[iVar5] = iVar3;
        iVar5 = iVar5 + 1;
      }
      sVar4 = sVar4 + 1;
      iVar3 = (int)sVar4;
    } while (iVar3 < *(int *)(iVar1 + 0x378));
  }
  return iVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
