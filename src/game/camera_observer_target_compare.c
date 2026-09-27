// camera_observer_target_compare  (Ghidra: FUN_0045a4a0; renamed per symbols/review_queue.txt)
// address 0x45a4a0, size 142 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x45a4a0..0x45a52d)
// evidence: types/game.h observer_target_candidate (0x38 bytes: object 0x00, point 0x04,
//   offset 0x10, direction 0x1c, distance 0x28, angle 0x2c, weight_primary 0x30,
//   weight_secondary 0x34); the dword indices below (0xc,0xd,0xa,0xb,0) match those byte
//   offsets exactly. qsort comparator used by camera_observer_find_best_target (0x459a00).
// register convention: both entries are the recognized stack parameters (param_1, param_2);
//   no register arguments (this is a plain qsort callback).
//   // blam-cc: stack -> a, b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// Orders candidates by weight_primary (descending), then weight_secondary (descending), then
// distance (ascending), then angle (ascending), then object index (ascending).
int32_t camera_observer_target_compare(const observer_target_candidate *a, const observer_target_candidate *b)
{
    if (b->weight_primary < a->weight_primary) {
        return -1;
    }
    if (b->weight_primary <= a->weight_primary) {
        if (b->weight_secondary < a->weight_secondary) {
            return -1;
        }
        if (b->weight_secondary <= a->weight_secondary) {
            if (a->distance < b->distance) {
                return -1;
            }
            if (a->distance <= b->distance) {
                if (a->angle < b->angle) {
                    return -1;
                }
                if (a->angle <= b->angle) {
                    return (int32_t)((a->object & 0xffff) - (b->object & 0xffff));
                }
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45a4a0), from tools/pack.py 0x45a4a0:

int FUN_0045a4a0(uint *param_1,uint *param_2)

{
  if ((float)param_2[0xc] < (float)param_1[0xc]) {
    return -1;
  }
  if ((float)param_2[0xc] <= (float)param_1[0xc]) {
    if ((float)param_2[0xd] < (float)param_1[0xd]) {
      return -1;
    }
    if ((float)param_2[0xd] <= (float)param_1[0xd]) {
      if ((float)param_1[10] < (float)param_2[10]) {
        return -1;
      }
      if ((float)param_1[10] <= (float)param_2[10]) {
        if ((float)param_1[0xb] < (float)param_2[0xb]) {
          return -1;
        }
        if ((float)param_1[0xb] <= (float)param_2[0xb]) {
          return (*param_1 & 0xffff) - (*param_2 & 0xffff);
        }
      }
    }
  }
  return 1;
}
#endif
