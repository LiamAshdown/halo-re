// waypoint_table_quantize_initialize  (Ghidra: FUN_004eb560; named per this rewrite)
// address 0x4eb560, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md ("Initializes and quantizes a table of 3D points
// (e.g. waypoints), stopping at a NaN sentinel or the table's element count.");
// vector3d_quantize.c (this batch, 0x4eb4a0, the per-point callee).
// register convention: none beyond the stack-recognized parameter.
// UNSURE (extensive, callers=0 in this batch): FUN_004eb4a0/vector3d_quantize is called here with
// zero visible arguments; the arguments this rewrite supplies (range = table's first two floats,
// max_level = the first group's derived level count, point = the current table entry, writing
// back in place) are a plausible reconstruction based on that function's own parameter shapes,
// not a disassembly-confirmed value. The two isnan checks on table[0]/table[1] are computed and
// then never used (matching the "no invented behaviour" rule, they are preserved as dead work).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t isnan(double x); // Ghidra's __isnan() pseudo-function


extern uint32_t vector3d_quantize(int32_t *out_indices, real *range, uint32_t max_level,
    real *point); // this module, 0x4eb4a0; UNSURE: arguments, see file header

// Lazily fills in table's two derived max-level fields, then quantizes each waypoint in table
// (up to table->count, stopping early at the first NaN-containing point). Returns 1 on success
// (including when count <= 0), 0 if a NaN sentinel was found before count entries were processed.
uint32_t waypoint_table_quantize_initialize(uint8_t *container)
{
    waypoint_table *table;
    int32_t index;
    real_point3d *point;

    table = *(waypoint_table **)(container + 0x58);
    (void)isnan((double)table->range_min);
    if (!isnan((double)table->range_min)) {
        isnan((double)table->range_max);
    }
    if (table->max_level_a == 0.0f) {
        table->max_level_a = (real)((1 << (((uint8_t *)&table->bits_a)[0] & 0x1f)) - 1);
    }
    if (table->max_level_b == 0.0f) {
        table->max_level_b = (real)((1 << (((uint8_t *)&table->bits_b)[0] & 0x1f)) - 1);
    }
    if ((int32_t)table->count_as_float < 1) {
        return 1;
    }
    index = 0;
    point = table->points;
    while (!isnan((double)point->x) && !isnan((double)point->y) && !isnan((double)point->z)) {
        vector3d_quantize((int32_t *)point, &table->range_min, (uint32_t)table->max_level_a,
            (real *)point);
        index = index + 1;
        point = point + 1;
        if ((int32_t)table->count_as_float <= index) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4eb560), from tools/pack.py 0x4eb560:

undefined4 FUN_004eb560(int param_1)

{
  float *pfVar1;
  int iVar2;
  float *pfVar3;
  int local_c;

  pfVar1 = *(float **)(param_1 + 0x58);
  iVar2 = __isnan((double)*pfVar1);
  if (iVar2 == 0) {
    __isnan((double)pfVar1[1]);
  }
  if (pfVar1[3] == 0.0) {
    pfVar1[3] = (float)((1 << (SUB41(pfVar1[2],0) & 0x1f)) + -1);
  }
  if (pfVar1[5] == 0.0) {
    pfVar1[5] = (float)((1 << (SUB41(pfVar1[4],0) & 0x1f)) + -1);
  }
  if ((int)pfVar1[6] < 1) {
    return 1;
  }
  local_c = 0;
  pfVar3 = pfVar1 + 9;
  while (((iVar2 = __isnan((double)pfVar3[-2]), iVar2 == 0 &&
          (iVar2 = __isnan((double)pfVar3[-1]), iVar2 == 0)) &&
         (iVar2 = __isnan((double)*pfVar3), iVar2 == 0))) {
    FUN_004eb4a0();
    local_c = local_c + 1;
    pfVar3 = pfVar3 + 3;
    if ((int)pfVar1[6] <= local_c) {
      return 1;
    }
  }
  return 0;
}
#endif
