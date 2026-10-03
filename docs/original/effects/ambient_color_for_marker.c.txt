// ambient_color_for_marker  (Ghidra: FUN_0053f940, still unnamed there; named directly by
//   types/effects.h: "ambient_color_for_marker 0x53f940 blends that against the wind colour of
//   the weather palette row the marker sits in")
// address 0x53f940, size 303 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x53f940..0x53fa6e; offsets probed) (see UNSURE)
// evidence: types/effects.h weather_particle_system_state (active, magnitude, direction_i/j/k);
//   types/tags.h Wind (local_variation_weight +0x10, local_variation_rate +0x14, damping +0x18);
//   src/math's global_origin3d_pointer default.
// register convention: weather palette row index in AX (in_AX, set up by the caller --
//   src/effects/ambient_color_marker_visible.c's FUN_0053ec30 result is the only nearby
//   candidate); output ColorRGB pointer in EDI (unaff_EDI); a position pointer and a bit flag
//   pair are Ghidra's own recognized stack parameters (param_1, param_2), with param_1 never
//   read directly in this function's own body -- only forwarded into ambient_color_sample,
//   which is the only reading consistent with that callee needing a position at all.
//   // blam-cc: in_AX -> weather_row, unaff_EDI -> out, stack -> (position, flags)
// UNSURE: the exact roles of `flags` bits 0 and 1 (skip local variation vs. apply damping) are
//   inferred from which branch each one gates, not from any external evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t weather_particle_system_count;                // 0x00746b84
extern weather_particle_system_state weather_wind_states[8]; // 0x00746b88
extern ScenarioStructureBSP *global_structure_bsp;
extern tag_instance *tag_instances;                           // 0x0087bc14
extern const real_point3d *global_origin3d_pointer;          // 0x00696714 -> 0x0065c230

extern void ambient_color_sample(ColorRGB *out, real_point3d *position, real hash_scale,
                                  real intensity); // 0x53fc80, this module

// Computes the ambient colour for a marker: if the marker's weather palette row has an active
// Wind, blends an ambient_color_sample against that row's wind direction (optionally damped),
// otherwise (or when the row is out of range / inactive) returns the default origin colour.
void ambient_color_for_marker(int16_t weather_row, real_point3d *position, uint8_t flags,
    real_vector3d *out)
{
    if (weather_row >= 0 && weather_row < weather_particle_system_count &&
        *((uint8_t *)&weather_wind_states[0] + weather_row * 0x20) != 0) {
        ScenarioStructureBSPWeatherPalette *palette_row =
            (ScenarioStructureBSPWeatherPalette *)((uint8_t *)global_structure_bsp->weather_palette.pointer) +
            weather_row;
        Wind *wind_tag = (Wind *)tag_instances[palette_row->wind.tag_id.index].data;
        weather_particle_system_state *wind = &weather_wind_states[weather_row];
        real local_variation = (flags & 1) == 0 ? wind_tag->local_variation_weight : 0.0f;
        ColorRGB sample;

        ambient_color_sample(&sample, position, wind_tag->local_variation_rate,
            wind_tag->local_variation_weight * wind->magnitude);

        local_variation = 1.0f - local_variation;
        out->i = local_variation * wind->direction_i + sample.red;
        out->j = local_variation * wind->direction_j + sample.green;
        out->k = local_variation * wind->direction_k + sample.blue;

        if ((flags & 2) != 0) {
            real damping = 1.0f - wind_tag->damping;
            out->i = damping * out->i;
            out->j = damping * out->j;
            out->k = damping * out->k;
        }
        return;
    }

    // types/math.h declares this as const real_point3d *; the three floats are the same.
    *out = *(const real_vector3d *)global_origin3d_pointer;
}

#if 0
Original Ghidra decompilation (0x53f940):

void FUN_0053f940(undefined4 param_1,byte param_2)

{
  int iVar1;
  undefined *puVar2;
  short in_AX;
  int iVar3;
  float *unaff_EDI;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar2 = PTR_DAT_00696714;
  if ((-1 < in_AX) && (in_AX < DAT_00746b84)) {
    iVar3 = (int)in_AX;
    if ((&DAT_00746b88)[iVar3 * 0x20] != '\0') {
      iVar1 = *(int *)((*(uint *)(iVar3 * 0xf0 + 0x8c + *(int *)(DAT_00746f9c + 0x1b8)) & 0xffff) *
                       0x20 + 0x14 + DAT_0087bc14);
      if ((param_2 & 1) == 0) {
        local_10 = *(float *)(iVar1 + 0x10);
      }
      else {
        local_10 = 0.0;
      }
      FUN_0053fc80(*(undefined4 *)(iVar1 + 0x14),
                   *(float *)(iVar1 + 0x10) * (float)(&DAT_00746b98)[iVar3 * 8]);
      local_10 = 1.0 - local_10;
      *unaff_EDI = local_10 * (float)(&DAT_00746b9c)[iVar3 * 8] + local_c;
      unaff_EDI[1] = local_10 * (float)(&DAT_00746ba0)[iVar3 * 8] + local_8;
      unaff_EDI[2] = local_10 * (float)(&DAT_00746ba4)[iVar3 * 8] + local_4;
      if ((param_2 & 2) != 0) {
        *unaff_EDI = (1.0 - *(float *)(iVar1 + 0x18)) * *unaff_EDI;
        unaff_EDI[1] = (1.0 - *(float *)(iVar1 + 0x18)) * unaff_EDI[1];
        unaff_EDI[2] = (1.0 - *(float *)(iVar1 + 0x18)) * unaff_EDI[2];
      }
      return;
    }
    *unaff_EDI = *(float *)PTR_DAT_00696714;
    unaff_EDI[1] = *(float *)(puVar2 + 4);
    unaff_EDI[2] = *(float *)(puVar2 + 8);
    return;
  }
  *unaff_EDI = *(float *)PTR_DAT_00696714;
  unaff_EDI[1] = *(float *)(puVar2 + 4);
  unaff_EDI[2] = *(float *)(puVar2 + 8);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
