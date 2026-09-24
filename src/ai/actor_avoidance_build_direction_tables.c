// actor_avoidance_build_direction_tables  (Ghidra: actor_avoidance_build_direction_tables, renamed)
// address 0x41a2d0, size 325 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: called exactly once, from ai_initialize_for_new_map, and it is the only writer
// of the three constant tables types/ai.h lists as owned by this module
// (0x00880380 16 rows of 7 floats, 0x00880540 8 rows of 3 floats, 0x008805a0 9 rows of 7
// floats). Each 7-float row is { scale, radius * unit.i, radius * unit.j, radius * unit.k,
// cos(elevation), sin(elevation) * unit.j, sin(elevation) * unit.k }, built from the .rdata
// angle and radius arrays at 0x006556a4 / 0x006556c8 / 0x006556ec (nine entries each, the
// 0x8805a0 table) and 0x00655714 / 0x00655734 / 0x0065573c (eight and two entries, the
// 0x880380 table).
// register convention: no arguments.
// blam-cc: (no arguments)
// UNSURE: the second loop writes row[4] twice -- once as sin(elevation) * circle[0], which is
// always zero because circle[0] is zero, and immediately after as cos(elevation). That is
// what the original does; the first store is dead but is kept here so the two loops read the
// same way.
// UNSURE: the row-interleaving of the second loop (outer index 0 fills rows 0, 2, 4 .. 14 and
// outer index 1 fills rows 1, 3 .. 15) comes from the pointer strides -- 0xe floats per inner
// step and 7 floats per outer step -- not from any comment in the binary.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double cos(double x); // FCOS
extern double sin(double x); // FSIN

// The three tables this function owns.
extern float actor_avoidance_samples_a[16][7]; // 0x00880380
extern float actor_avoidance_circle[8][3];     // 0x00880540
extern float actor_avoidance_samples_b[9][7];  // 0x008805a0

// The .rdata source constants (not owned by this module).
extern const float actor_avoidance_b_radius[9];      // 0x006556a4
extern const float actor_avoidance_b_elevation[9];   // 0x006556c8, in units of pi/60
extern const float actor_avoidance_b_bearing[9];     // 0x006556ec, radians
extern const float actor_avoidance_a_bearing[8];     // 0x00655714, radians
extern const float actor_avoidance_a_radius[2];      // 0x00655734
extern const float actor_avoidance_a_elevation[2];   // 0x0065573c, radians

// blam-cc: (no arguments)
void actor_avoidance_build_direction_tables(void)
{
    int32_t i;
    int32_t j;
    float bearing;
    float elevation;
    float radius;
    float sin_bearing;
    float cos_bearing;
    float sin_elevation;
    float cos_elevation;

    for (i = 0; i < 9; i++) {
        bearing = actor_avoidance_b_bearing[i];
        sin_bearing = (float)sin((double)bearing);
        cos_bearing = (float)cos((double)bearing);
        elevation = actor_avoidance_b_elevation[i] * 0.05235988f;
        sin_elevation = (float)sin((double)elevation);
        cos_elevation = (float)cos((double)elevation);
        radius = actor_avoidance_b_radius[i];

        actor_avoidance_samples_b[i][0] = 1.0f;
        actor_avoidance_samples_b[i][1] = 0.0f;
        actor_avoidance_samples_b[i][2] = cos_bearing * radius * 0.7f;
        actor_avoidance_samples_b[i][3] = sin_bearing * radius * 0.7f;
        actor_avoidance_samples_b[i][4] = cos_elevation;
        actor_avoidance_samples_b[i][5] = sin_elevation * cos_bearing;
        actor_avoidance_samples_b[i][6] = sin_elevation * sin_bearing;
    }

    for (i = 0; i < 2; i++) {
        sin_elevation = (float)sin((double)actor_avoidance_a_elevation[i]);
        cos_elevation = (float)cos((double)actor_avoidance_a_elevation[i]);
        radius = actor_avoidance_a_radius[i];

        for (j = 0; j < 8; j++) {
            float *row = actor_avoidance_samples_a[j * 2 + i];

            bearing = actor_avoidance_a_bearing[j];
            actor_avoidance_circle[j][0] = 0.0f;
            actor_avoidance_circle[j][1] = (float)cos((double)bearing);
            actor_avoidance_circle[j][2] = (float)sin((double)bearing);

            row[0] = 0.7f;
            row[1] = radius * actor_avoidance_circle[j][0];
            row[2] = radius * actor_avoidance_circle[j][1];
            row[3] = radius * actor_avoidance_circle[j][2];
            row[4] = sin_elevation * actor_avoidance_circle[j][0]; // dead: circle[j][0] is 0
            row[5] = sin_elevation * actor_avoidance_circle[j][1];
            row[6] = sin_elevation * actor_avoidance_circle[j][2];
            row[4] = cos_elevation;
        }
    }
}

#if 0
Original Ghidra decompilation (0x41a2d0):

void FUN_0041a2d0(void)

{
  float fVar1;
  float fVar2;
  undefined4 *puVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  float *pfVar7;
  float *pfVar8;
  int iVar9;
  float10 fVar10;
  float10 fVar11;
  float10 fVar12;
  float10 fVar13;
  int local_4;

  iVar6 = 9;
  puVar3 = &DAT_008805a4;
  iVar9 = 0;
  do {
    fVar1 = *(float *)((int)&DAT_006556ec + iVar9);
    puVar3[-1] = 0x3f800000;
    fVar10 = (float10)fsin((float10)fVar1);
    *puVar3 = 0;
    iVar6 = iVar6 + -1;
    fVar11 = (float10)fcos((float10)*(float *)((int)&DAT_006556ec + iVar9));
    fVar12 = (float10)*(float *)((int)&DAT_006556c8 + iVar9) * (float10)0.05235988;
    fVar13 = (float10)fsin(fVar12);
    puVar3[1] = (float)(fVar11 * (float10)*(float *)((int)&DAT_006556a4 + iVar9) * (float10)0.7);
    puVar3[2] = (float)(fVar10 * (float10)*(float *)((int)&DAT_006556a4 + iVar9) * (float10)0.7);
    fVar12 = (float10)fcos(fVar12);
    puVar3[3] = (float)fVar12;
    puVar3[4] = (float)((float10)(float)fVar13 * fVar11);
    puVar3[5] = (float)((float10)(float)fVar13 * fVar10);
    puVar3 = puVar3 + 7;
    iVar9 = iVar9 + 4;
  } while (iVar6 != 0);
  pfVar8 = (float *)&DAT_0088038c;
  iVar9 = 0;
  local_4 = 2;
  do {
    fVar10 = (float10)fsin((float10)*(float *)((int)&DAT_0065573c + iVar9));
    iVar6 = 8;
    fVar11 = (float10)fcos((float10)*(float *)((int)&DAT_0065573c + iVar9));
    fVar1 = *(float *)((int)&DAT_00655734 + iVar9);
    pfVar4 = (float *)&DAT_00880548;
    pfVar5 = pfVar8;
    pfVar7 = (float *)&DAT_00655714;
    do {
      fVar2 = *pfVar7;
      pfVar4[-2] = 0.0;
      fVar12 = (float10)fcos((float10)fVar2);
      pfVar5[-3] = 0.7;
      iVar6 = iVar6 + -1;
      pfVar4[-1] = (float)fVar12;
      fVar12 = (float10)fsin((float10)*pfVar7);
      *pfVar4 = (float)fVar12;
      pfVar5[-2] = fVar1 * pfVar4[-2];
      pfVar5[-1] = fVar1 * pfVar4[-1];
      *pfVar5 = fVar1 * *pfVar4;
      pfVar5[1] = (float)(fVar10 * (float10)pfVar4[-2]);
      pfVar5[2] = (float)(fVar10 * (float10)pfVar4[-1]);
      pfVar5[3] = (float)(fVar10 * (float10)*pfVar4);
      pfVar5[1] = (float)fVar11;
      pfVar4 = pfVar4 + 3;
      pfVar5 = pfVar5 + 0xe;
      pfVar7 = pfVar7 + 1;
    } while (iVar6 != 0);
    iVar9 = iVar9 + 4;
    pfVar8 = pfVar8 + 7;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return;
}
#endif
