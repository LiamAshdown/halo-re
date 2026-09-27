// effect_random_direction_vector  (Ghidra: FUN_00451450, still unnamed there; named from its own
// summary in out/phase4/effects_functions.md: "Picks a random spawn direction vector for a
// particle system from a precomputed direction table, scaled by a random magnitude, or a
// default direction if the magnitude is zero")
// address 0x451450, size 168 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump 0x451450..0x4514f7; property call FIXED) (see UNSURE)
// evidence: src/math/vector3d_randomize_direction.c establishes sphere_point_table /
// sphere_point_table_count; types/math.h global_origin3d_pointer (0x00696714 -> 0x0065c230).
// register convention: effect* and the two random-value bit-set/self arguments this forwards to
// effect_property_random_value are fully elided; only the seed pointer, output vector and the
// two magnitude bounds are Ghidra's own recognised stack parameters.
// UNSURE: as with effect_random_velocity_vector, the effect_property_random_value call here
// shows only 3 of its 7 real arguments; the rest are passed as 0/self here, a guess.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026
extern const real_point3d *global_origin3d_pointer; // 0x00696714 -> 0x0065c230, math module

extern real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset,
    uint32_t b_bitset, random_seed *seed, real base_min, real base_max); // 0x451290, this module

// Rolls a random magnitude in [min, max); if non-zero, scales a random unit vector out of
// sphere_point_table by it, otherwise returns the global origin point.
// FIXED (objdump 0x451450..0x451469): effect_property_random_value is called with EDX = 3 (the property bit)
//   and EBX / ESI / EDI passed straight through from the caller (the effect and its part's a / b scale bits;
//   effect_event_apply 0x452f5e..0x452f61). The draft passed bit 0 and zeros.
// blam-cc: stack -> seed, out, min, max; EBX -> self, ESI -> a_bitset, EDI -> b_bitset
void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max,
    effect *self, uint32_t a_bitset, uint32_t b_bitset)
{
    real magnitude = effect_property_random_value(3, self, a_bitset, b_bitset, seed, min, max);

    if (magnitude != 0.0f) {
        int16_t index;

        *seed = *seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((*seed >> k_random_value_shift) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);

        out->x = magnitude * sphere_point_table[index].x;
        out->y = magnitude * sphere_point_table[index].y;
        out->z = magnitude * sphere_point_table[index].z;
    } else {
        *out = *global_origin3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x451450):

void FUN_00451450(uint *param_1,float *param_2,undefined4 param_3,undefined4 param_4)

{
  float *pfVar1;
  undefined *puVar2;
  uint uVar3;
  float10 fVar4;

  fVar4 = (float10)particle_system_property_random_value(param_1,param_3,param_4);
  puVar2 = PTR_DAT_00696714;
  if (fVar4 != (float10)0.0) {
    uVar3 = *param_1 * 0x19660d + 0x3c6ef35f;
    *param_1 = uVar3;
    pfVar1 = (float *)(DAT_006b7af4 + (short)((uVar3 >> 0x10) * (int)DAT_006b7af8 >> 0x10) * 0xc);
    *param_2 = *pfVar1;
    param_2[1] = pfVar1[1];
    param_2[2] = pfVar1[2];
    *param_2 = (float)(fVar4 * (float10)*param_2);
    param_2[1] = (float)(fVar4 * (float10)param_2[1]);
    param_2[2] = (float)(fVar4 * (float10)param_2[2]);
    return;
  }
  *param_2 = *(float *)PTR_DAT_00696714;
  param_2[1] = *(float *)(puVar2 + 4);
  param_2[2] = *(float *)(puVar2 + 8);
  return;
}
#endif
