// effect_random_direction_from_table  (Ghidra: FUN_004505e0; named per out/phase2/results/
// effects_00.json "effect_random_direction_from_table", confidence 0.45)
// address 0x4505e0, size 78 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: src/math/vector3d_randomize_direction.c already establishes sphere_point_table /
// sphere_point_table_count (0x006b7af4 / 0x006b7af8) as 1026 real_point3d unit vectors.
// register convention: output vector in EAX (in_EAX).
//   // blam-cc: EAX -> out

#include "tags.h"
#include "math.h"

extern random_seed effect_random_seed;    // 0x00719cd4
extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026

// Picks a pseudo-random unit vector out of the shared quasi-uniform sphere point table.
void effect_random_direction_from_table(real_point3d *out)
{
    int16_t index;

    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    index = (int16_t)(((effect_random_seed >> k_random_value_shift) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);
    *out = sphere_point_table[index];
}

#if 0
Original Ghidra decompilation (0x4505e0):

void FUN_004505e0(void)

{
  undefined4 *puVar1;
  undefined4 *in_EAX;

  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  puVar1 = (undefined4 *)
           (DAT_006b7af4 + (short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10) * 0xc);
  *in_EAX = *puVar1;
  in_EAX[1] = puVar1[1];
  in_EAX[2] = puVar1[2];
  return;
}
#endif
