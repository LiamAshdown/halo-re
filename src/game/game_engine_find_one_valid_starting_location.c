// game_engine_find_one_valid_starting_location  (Ghidra: FUN_00461180; renamed -- it is a
// single-result wrapper around game_engine_find_valid_starting_locations, not a discard-only
// trigger)
// address 0x461180, size 40 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Thin wrapper around the starting-location search that
// discards the results, apparently used only to trigger its side effects" -- but it plainly
// returns the one index found, or -1); this batch's game_engine_find_valid_starting_locations
// (0x461080), whose own header explains the team/type parameter swap this wrapper inherits
// unchanged.
// register convention: type filter in ECX (in_CX), team filter in EDX (in_DX), the search origin
// in EBX (unaff_EBX, forwarded straight through -- this wrapper never touches it itself);
// param_1/param_2 are this function's own stack parameters.
//   // blam-cc: ECX -> type, EDX -> team, unaff_EBX -> origin, stack -> max_horizontal_dist,
//   //          max_height_delta

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results); // 0x461080, this batch

// blam-cc: ECX -> type, EDX -> team, unaff_EBX -> origin, stack -> max_horizontal_dist, max_height_delta
int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team,
    real_point3d *origin, float max_horizontal_dist, float max_height_delta)
{
    int32_t result = -1;
    game_engine_find_valid_starting_locations(origin, max_horizontal_dist, max_height_delta,
        team, type, 1, &result);
    return result;
}

#if 0
Original Ghidra decompilation (0x461180), from tools/pack.py 0x461180:

int FUN_00461180(float param_1,float param_2)

{
  short in_CX;
  short in_DX;
  int local_4;
  
  local_4 = -1;
  game_engine_find_valid_starting_locations(param_1,param_2,in_DX,in_CX,1,&local_4);
  return local_4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
