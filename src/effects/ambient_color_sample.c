// ambient_color_sample  (Ghidra: FUN_0053fc80, still unnamed there; named directly by
//   types/effects.h: "ambient_color_sample 0x53fc80 samples and dithers three entries from the
//   ambient color probe grid to produce a smoothed, intensity-scaled RGB color")
// address 0x53fc80, size 219 bytes
// name confidence: 0.6   rewrite confidence: 0.2 (see UNSURE)
// evidence: types/effects.h ambient_noise_grid (3 by 8 by 8 real_vector3d at 0x00746284, band
//   stride 0x300, row stride 0x60), "weights the three by 0.1, 0.2 and 0.07 literals and scales
//   the sum by one third of the caller intensity"; src/math's global_origin3d_pointer default.
// register convention: output ColorRGB pointer in EAX (in_EAX); position pointer in EDX
//   (in_EDX); a hash-scale factor and intensity are Ghidra's own recognized stack parameters.
//   // blam-cc: in_EAX -> out, in_EDX -> position, stack -> (hash_scale, intensity)
// UNSURE: the per-band hash reads `position[band]` and `position[band + 1]` (the latter via a
// stack-relative expression Ghidra could not simplify); for band 2 that would read one float past
// a 3 float position, so this rewrite wraps it back to `position[0]`, which is a guess about the
// caller's real stack layout, not something the decompile proves. The "round to nearest int via
// adding 2^23" idiom is reproduced with a plain cast, which is only bit-identical for values
// already in range, not for the exact FPU rounding mode the original relies on. The 0.1/0.2/0.07
// band weights types/effects.h documents are set up as a local array in the decompile but never
// visibly multiplied anywhere in it (almost certainly a lost optimisation on Ghidra's part, since
// three grid samples summed unweighted would not need three different literals at all); applied
// here per the header's explicit claim rather than left out to match the letter of the
// (evidently incomplete) decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern int32_t weather_frame_counter;      // 0x00746f88
extern ambient_noise_grid ambient_noise;   // 0x00746284
extern const real_point3d *global_origin3d_pointer; // 0x00696714 -> 0x0065c230, math module

// Hashes a position and the current frame counter into one grid column per band (0.1/0.2/0.07
// weighted), sums the three sampled vectors on top of the default origin colour, and scales the
// result by one third of `intensity`.
void ambient_color_sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity)
{
    static const real k_band_weight[3] = {0.1f, 0.2f, 0.07f};
    const real *pos = (const real *)position;
    int band;
    // types/math.h declares this as const real_point3d *; the three floats are the same.
    real accum_r = global_origin3d_pointer->x;
    real accum_g = global_origin3d_pointer->y;
    real accum_b = global_origin3d_pointer->z;

    for (band = 0; band < 3; band++) {
        real next_component = pos[(band + 1) % 3]; // UNSURE, see file header
        real hashed = pos[band] + (real)weather_frame_counter * next_component * hash_scale;
        int32_t column = ((int32_t)((hashed < 0.0f ? -hashed : hashed) * 8.0f)) & 0x3f;
        int32_t index = band * 0x40 + column;
        const real_vector3d *entry = &ambient_noise.entries[0][0][0] + index;

        accum_r += entry->i * k_band_weight[band]; // UNSURE, see file header
        accum_g += entry->j * k_band_weight[band];
        accum_b += entry->k * k_band_weight[band];
    }

    {
        real scale = intensity * (1.0f / 3.0f);
        out->red = scale * accum_r;
        out->green = scale * accum_g;
        out->blue = scale * accum_b;
    }
}

#if 0
Original Ghidra decompilation (0x53fc80):

void FUN_0053fc80(float param_1,float param_2)

{
  float fVar1;
  undefined *puVar2;
  float *in_EAX;
  int iVar3;
  float *in_EDX;
  float *pfVar4;
  int iVar5;
  int iVar6;
  float local_c [3];

  puVar2 = PTR_DAT_00696714;
  fVar1 = param_2 * 0.33333334;
  *in_EAX = *(float *)PTR_DAT_00696714;
  in_EAX[1] = *(float *)(puVar2 + 4);
  in_EAX[2] = *(float *)(puVar2 + 8);
  iVar6 = 0;
  local_c[0] = 0.1;
  local_c[1] = 0.2;
  local_c[2] = 0.07;
  iVar5 = 3;
  pfVar4 = in_EDX;
  do {
    param_2._0_1_ =
         SUB41(ABS(((float)DAT_00746f88 *
                    *(float *)(&stack0xfffffff0 + -(int)in_EDX + (int)(pfVar4 + 1)) * param_1 +
                   *pfVar4) * 8.0) + 8388608.0,0);
    iVar3 = (short)(param_2._0_1_ & 0x3f) + iVar6;
    iVar6 = iVar6 + 0x40;
    iVar5 = iVar5 + -1;
    *in_EAX = (float)(&DAT_00746284)[iVar3 * 3] + *in_EAX;
    in_EAX[1] = (float)(&DAT_00746288)[iVar3 * 3] + in_EAX[1];
    in_EAX[2] = (float)(&DAT_0074628c)[iVar3 * 3] + in_EAX[2];
    pfVar4 = pfVar4 + 1;
  } while (iVar5 != 0);
  *in_EAX = fVar1 * *in_EAX;
  in_EAX[1] = fVar1 * in_EAX[1];
  in_EAX[2] = fVar1 * in_EAX[2];
  return;
}
#endif
