// game_engine_find_nearest_unused_type4_location  (Ghidra: FUN_0046d520; named per its summary)
// address 0x46d520, size 167 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Finds the nearest (or, if no reference point is
//   given, the first) unused type-4 scenario starting location, excluding indices already
//   claimed"); types/tags.h ScenarioNetgameFlags (0x94 stride, type at +0x10, position at +0x00).
// register convention: excluded-index array in param_1 (stack); its count in unaff_EDI;
//   reference point in unaff_EBX.
//   // blam-cc: stack -> excluded_indices, EDI -> excluded_count, EBX -> reference_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: stack -> excluded_indices, EDI -> excluded_count, EBX -> reference_point
// Scans every type-4 ScenarioNetgameFlags entry not already present in excluded_indices[0..
// excluded_count-1]. If reference_point is NULL, returns the first such entry's index.
// Otherwise returns the index of the closest one, or -1 if none qualify.
int32_t game_engine_find_nearest_unused_type4_location(int32_t *excluded_indices,
    int32_t excluded_count, real_point3d *reference_point)
{
    int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
    float best_distance = 1e+06f;
    int32_t best_index = -1;
    int32_t i;

    for (i = 0; i < flag_count; i++) {
        int32_t j;
        uint8_t excluded = 0;

        if (flags[i].type != 4) {
            continue;
        }
        for (j = 0; j < excluded_count; j++) {
            if (excluded_indices[j] == i) {
                excluded = 1;
                break;
            }
        }
        if (excluded) {
            continue;
        }
        if (reference_point == (real_point3d *)0) {
            return i;
        }
        {
            float dx = flags[i].position.x - reference_point->x;
            float dy = flags[i].position.y - reference_point->y;
            float dz = flags[i].position.z - reference_point->z;
            float dist = dy * dy + dz * dz + dx * dx;
            if (dist < best_distance) {
                best_distance = dist;
                best_index = i;
            }
        }
    }
    return best_index;
}

#if 0
Original Ghidra decompilation (0x46d520), from tools/pack.py 0x46d520:

int FUN_0046d520(int param_1)

{
  float fVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float *unaff_EBX;
  int unaff_EDI;
  float local_8;
  int local_4;

  iVar3 = 0;
  local_8 = 1e+06;
  local_4 = -1;
  if (0 < *(int *)(global_scenario + 0x378)) {
    pfVar4 = *(float **)(global_scenario + 0x37c);
    do {
      if (*(short *)(pfVar4 + 4) == 4) {
        iVar2 = 0;
        if (0 < unaff_EDI) {
          do {
            if (iVar3 == *(int *)(param_1 + iVar2 * 4)) goto LAB_0046d5aa;
            iVar2 = iVar2 + 1;
          } while (iVar2 < unaff_EDI);
        }
        if (unaff_EBX == (float *)0x0) {
          return iVar3;
        }
        fVar1 = (pfVar4[1] - unaff_EBX[1]) * (pfVar4[1] - unaff_EBX[1]) +
                (pfVar4[2] - unaff_EBX[2]) * (pfVar4[2] - unaff_EBX[2]) +
                (*pfVar4 - *unaff_EBX) * (*pfVar4 - *unaff_EBX);
        if (fVar1 < local_8) {
          local_8 = fVar1;
          local_4 = iVar3;
        }
      }
LAB_0046d5aa:
      iVar3 = iVar3 + 1;
      pfVar4 = pfVar4 + 0x25;
    } while (iVar3 < *(int *)(global_scenario + 0x378));
  }
  return local_4;
}
#endif
