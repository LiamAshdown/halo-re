// ambient_color_sample  (Ghidra: FUN_0053fc80, still unnamed there; named directly by
//   types/effects.h: "ambient_color_sample 0x53fc80 samples and dithers three entries from the
//   ambient color probe grid to produce a smoothed, intensity-scaled RGB color")
// address 0x53fc80, size 219 bytes
// name confidence: 0.6   rewrite confidence: 0.85
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

// REWRITTEN 2026-09-27 (static loop) from objdump 0x53fc80..0x53fd5a. For each of the three bands i:
//   hashed = position[i] + weather_frame_counter * w[i] * hash_scale   (w = 0.1, 0.2, 0.07 -- TIME factors)
//   column = low 6 bits of (float)(|hashed * 8| + 2^23)                (the 2^23 trick: round to nearest)
//   out   += noise_grid[band i][column]                                 (unweighted)
// then out *= intensity / 3. The draft used the weights as accumulation weights, mixed in the NEXT position
// component and truncated the column.
void ambient_color_sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity)
    // blam-cc: EAX -> out, EDX -> position, stack -> hash_scale, intensity
{
    static const real k_band_time_scale[3] = {0.1f, 0.2f, 0.07f};
    const real *pos = (const real *)position;
    const real_vector3d *grid = &ambient_noise.entries[0][0][0];
    real scale = intensity * 0.33333334f; // 0x672b5c
    int band;

    out->red = global_origin3d_pointer->x;
    out->green = global_origin3d_pointer->y;
    out->blue = global_origin3d_pointer->z;

    for (band = 0; band < 3; band++) {
        real hashed = ((real)weather_frame_counter * k_band_time_scale[band] * hash_scale + pos[band]) * 8.0f;
        real rounded;
        uint32_t bits;
        int32_t index;

        hashed = hashed < 0.0f ? -hashed : hashed; // and dword,0x7fffffff
        rounded = hashed + 8388608.0f;             // 0x672b58
        bits = *(uint32_t *)&rounded;
        index = band * 0x40 + (int32_t)(bits & 0x3f);
        out->red += grid[index].i;
        out->green += grid[index].j;
        out->blue += grid[index].k;
    }

    out->red = scale * out->red;
    out->green = scale * out->green;
    out->blue = scale * out->blue;
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
