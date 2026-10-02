// game_engine_ctf_point_within_team_flag_radius  (Ghidra: FUN_00468990; named per its summary)
// address 0x468990, size 77 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Tests whether a given point lies within a given
//   radius of a per-team position (interpreted here as the team's flag location)");
//   game_engine_ctf_respawn_team_flag.c / game_engine_ctf_reset_team_return_credit.c (this
//   batch) both treat 0x006b0e88 as a per-team real_point3d* the same way. The return value is
//   Ghidra's x87 FCOM-flag CONCAT artifact (fcom/fnstsw with a NaN-safe unordered test); narrowed
//   here to a plain boolean the same way src/objects/object_disconnect_from_map.c already
//   narrows an equivalent CONCAT31 packing -- strictly-greater excludes both the equal case and
//   any NaN operand, exactly as the original's bit tests (0x100 "less", 0x4000 "equal") do.
// register convention: radius on the stack (param_1); team in_EAX; point pointer in_ECX.
//   // blam-cc: stack -> radius, EAX -> team, ECX -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88

// blam-cc: stack -> radius, EAX -> team, ECX -> point
// Returns true when `point` is non-NULL, `team` has a configured flag stand position, and the
// squared distance between them is strictly less than radius^2 (NaN-safe).
uint8_t game_engine_ctf_point_within_team_flag_radius(float radius, int32_t team, real_point3d *point)
{
    real_point3d *flag_position;
    float dx, dy, dz, distance_squared, radius_squared;

    if (point == (real_point3d *)0) {
        return 0;
    }
    flag_position = ctf_team_flag_stand_position[team];
    if (flag_position == (real_point3d *)0) {
        return 0;
    }

    dx = flag_position->x - point->x;
    dy = flag_position->y - point->y;
    dz = flag_position->z - point->z;
    distance_squared = dz * dz + dy * dy + dx * dx;
    radius_squared = radius * radius;

    return (radius_squared > distance_squared) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x468990), from tools/pack.py 0x468990:

uint FUN_00468990(float param_1)

{
  float *pfVar1;
  float fVar2;
  uint in_EAX;
  float *in_ECX;

  if (in_ECX != (float *)0x0) {
    pfVar1 = (float *)(&DAT_006b0e88)[in_EAX];
    in_EAX = 0;
    if (pfVar1 != (float *)0x0) {
      fVar2 = (pfVar1[2] - in_ECX[2]) * (pfVar1[2] - in_ECX[2]) +
              (pfVar1[1] - in_ECX[1]) * (pfVar1[1] - in_ECX[1]) +
              (*pfVar1 - *in_ECX) * (*pfVar1 - *in_ECX);
      param_1 = param_1 * param_1;
      in_EAX = CONCAT31((int3)(CONCAT22((short)((uint)pfVar1 >> 0x10),
                                        (ushort)(param_1 < fVar2) << 8 |
                                        (ushort)(NAN(param_1) || NAN(fVar2)) << 10 |
                                        (ushort)(param_1 == fVar2) << 0xe) >> 8),1);
      if ((in_EAX & 0x4100) == 0) {
        return in_EAX;
      }
    }
  }
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
