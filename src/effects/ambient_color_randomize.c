// ambient_color_randomize  (Ghidra: FUN_0053fa70, still unnamed there; named directly by
//   types/effects.h: "ambient_color_randomize 0x53fa70 periodically re-randomizes entries of the
//   ambient color probe grid and smoothly interpolates the transition between old and new
//   values")
// address 0x53fa70, size 518 bytes
// name confidence: 0.6   rewrite confidence: 0.3 (see UNSURE)
// evidence: types/effects.h ambient_noise_grid (3 bands x 8 rows x 8 columns at 0x00746284, band
//   stride 0x300, row stride 0x60; "rolls the column 0 entry of each of the 8 rows in each of
//   the 3 bands out of sphere_point_table and then fills columns 1 to 7 with
//   vector3d_catmull_rom_interpolate"); src/math/vector3d_randomize_direction.c establishes
//   sphere_point_table / sphere_point_table_count.
// register convention: __cdecl, no arguments.
// UNSURE: `vector3d_catmull_rom_interpolate`'s exact argument roles are reconstructed from its
//   name and the surrounding arithmetic (four control points and a 0..1-ish fraction), not from
//   an established prototype elsewhere in the codebase; the fourth control point address
//   (`&ambient_noise + ((phase & 7) + column) * 0x18`) uses a stride of 0x18 (6 floats), which
//   does not match this grid's own 0xc (3 float) column stride -- kept exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern ambient_noise_grid ambient_noise; // 0x00746284
extern real_point3d *sphere_point_table; // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count; // 0x006b7af8, 1026
extern random_seed random_seed_global;   // 0x00719cd0, math module

extern void vector3d_catmull_rom_interpolate(void *out, void *p0, real t0, real t1, real t); // 0x447080,
                                    // UNSURE signature, see file header

// Re-randomizes column 0 of every row of every band in the ambient noise grid by rolling a fresh
// sphere_point_table direction, then rebuilds columns 1..7 of every row as a Catmull-Rom
// interpolation toward the next row's column 0, giving each row a smooth transition sequence.
void ambient_color_randomize(void)
{
    int band, row, column;

    for (band = 0; band < k_ambient_noise_bands; band++) {
        for (row = 0; row < k_ambient_noise_rows; row++) {
            random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
            {
                int16_t index = (int16_t)(((random_seed_global >> k_random_value_shift) *
                    (uint32_t)(int32_t)sphere_point_table_count) >> 16);
                ambient_noise.entries[band][row][0] = *(real_vector3d *)&sphere_point_table[index];
            }
        }
    }

    for (row = 0; row < 8; row++) {
        for (column = 1; column < 8; column++) {
            uint8_t phase = (uint8_t)(row - 1);

            for (band = 0; band < 3; band++) {
                vector3d_catmull_rom_interpolate(&ambient_noise.entries[band][row][column],
                    (uint8_t *)&ambient_noise + ((phase & 7) + band * 8) * 0x18, // UNSURE, see
                                    // file header
                    (real)(row - 1), 1.0f, (real)(column - 1) * 0.125f + (real)(row - 1));
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x53fa70):

void FUN_0053fa70(void)

{
  undefined4 *puVar1;
  char cVar2;
  byte bVar3;
  undefined4 *puVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int local_3c;
  undefined *local_38;
  undefined *local_34;
  int local_30;
  undefined4 *local_2c;
  int local_28;
  int local_24;
  int local_20;

  iVar7 = (int)DAT_006b7af8;
  local_2c = &DAT_00746284;
  local_30 = 8;
  uVar5 = random_seed_global;
  do {
    iVar6 = 3;
    puVar4 = local_2c;
    do {
      uVar5 = uVar5 * 0x19660d + 0x3c6ef35f;
      puVar1 = (undefined4 *)(DAT_006b7af4 + (short)((uVar5 >> 0x10) * iVar7 >> 0x10) * 0xc);
      random_seed_global = uVar5;
      *puVar4 = *puVar1;
      puVar4[1] = puVar1[1];
      iVar6 = iVar6 + -1;
      puVar4[2] = puVar1[2];
      puVar4 = puVar4 + 0xc0;
    } while (iVar6 != 0);
    local_2c = local_2c + 0x18;
    local_30 = local_30 + -1;
  } while (local_30 != 0);
  local_3c = 0;
  cVar2 = '\x01';
  local_38 = &DAT_00746290;
  local_20 = 8;
  do {
    bVar3 = cVar2 - 2;
    local_30 = 1;
    cVar2 = cVar2 + '\x01';
    local_2c = (undefined4 *)local_38;
    local_24 = 7;
    do {
      iVar7 = 0;
      local_34 = (undefined *)local_2c;
      local_28 = 3;
      do {
        vector3d_catmull_rom_interpolate
                  (local_34,&DAT_00746284 + ((bVar3 & 7) + iVar7) * 0x18,(float)(local_3c + -1),
                   0x3f800000,(float)local_30 * 0.125 + (float)local_3c);
        local_34 = local_34 + 0x300;
        iVar7 = iVar7 + 8;
        local_28 = local_28 + -1;
      } while (local_28 != 0);
      local_30 = local_30 + 1;
      local_2c = (undefined4 *)((int)local_2c + 0xc);
      local_24 = local_24 + -1;
    } while (local_24 != 0);
    local_3c = local_3c + 1;
    local_38 = local_38 + 0x60;
    local_20 = local_20 + -1;
  } while (local_20 != 0);
  return;
}
#endif
